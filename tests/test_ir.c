#include "test_main.h"
#include "../src/ir.h"
#include "../src/opcode.h"

static inline bool parse(const char* src, ops_t* ops) {
  char path[64];
  memset(ops, 0, sizeof(*ops));

  if (!write_temp_file(src, path, sizeof(path))) {
    fprintf(stderr, "test_ir: could not create temp file\n");
    exit(1);
  }

  bool ok = generate_ops(path, ops);
  unlink(path);

  return ok;
}

static inline void test_empty_and_comments(void) {
  ops_t ops;

  CHECK(parse("", &ops));
  CHECK(ops.count == 0);
  free(ops.items);

  CHECK(parse("just a comment, no commands", &ops));
  CHECK(ops.count == 0);
  free(ops.items);
}

static inline void test_run_length_folding(void) {
  ops_t ops;
  
  CHECK(parse("+++", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_INC);
  CHECK(ops.items[0].operand == 3);
  free(ops.items);

  CHECK(parse("<<<<<", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_LEFT);
  CHECK(ops.items[0].operand == 5);
  free(ops.items);

  CHECK(parse(">>", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_RIGHT);
  CHECK(ops.items[0].operand == 2);
  free(ops.items);

  CHECK(parse("...", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_OUTPUT);
  CHECK(ops.items[0].operand == 3);
  free(ops.items);

  CHECK(parse(",,", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_INPUT);
  CHECK(ops.items[0].operand == 2);
  free(ops.items);
}

static inline void test_mixed_ops(void) {
  ops_t ops;

  CHECK(parse("++--", &ops));
  CHECK(ops.count == 2);
  CHECK(ops.items[0].kind == OP_INC);
  CHECK(ops.items[0].operand == 2);
  CHECK(ops.items[1].kind == OP_DEC);
  CHECK(ops.items[1].operand == 2);
  free(ops.items);

  CHECK(parse("+ a +\n+", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_INC);
  CHECK(ops.items[0].operand == 3);
  free(ops.items);  
}

static inline void test_loop_structure(void) {
  ops_t ops;

  CHECK(parse("+[>+<-]", &ops));
  CHECK(ops.count == 7);
  CHECK(ops.items[0].kind == OP_INC);
  CHECK(ops.items[1].kind == OP_JUMP_IF_ZERO);
  CHECK(ops.items[1].operand == 6); // index of matching ]
  CHECK(ops.items[2].kind == OP_RIGHT);
  CHECK(ops.items[3].kind == OP_INC);
  CHECK(ops.items[4].kind == OP_LEFT);
  CHECK(ops.items[5].kind == OP_DEC);
  CHECK(ops.items[6].kind == OP_JUMP_IF_NONZERO);
  CHECK(ops.items[6].operand == 1); // index of matching [
  free(ops.items);

  CHECK(parse("+[>+[>+<-]<-]", &ops));
  CHECK(ops.count == 13);
  CHECK(ops.items[1].kind == OP_JUMP_IF_ZERO);
  CHECK(ops.items[1].operand == 12);
  CHECK(ops.items[4].kind == OP_JUMP_IF_ZERO);
  CHECK(ops.items[4].operand == 9);
  CHECK(ops.items[9].kind == OP_JUMP_IF_NONZERO);
  CHECK(ops.items[9].operand == 4);
  CHECK(ops.items[12].kind == OP_JUMP_IF_NONZERO);
  CHECK(ops.items[12].operand == 1);
  free(ops.items);
}

static inline void test_clear_optimization(void) {
  ops_t ops;

  CHECK(parse("+[-]", &ops));
  CHECK(ops.count == 2);
  CHECK(ops.items[0].kind == OP_INC);
  CHECK(ops.items[1].kind == OP_CLEAR);
  free(ops.items);

  CHECK(parse("+[+]", &ops));
  CHECK(ops.count == 2);
  CHECK(ops.items[1].kind == OP_CLEAR);
  free(ops.items);

  CHECK(parse("+[---]", &ops));
  CHECK(ops.count == 2);
  CHECK(ops.items[1].kind == OP_CLEAR);
  free(ops.items);

  CHECK(parse("+[--]", &ops));
  CHECK(ops.count == 4);
  CHECK(ops.items[1].kind == OP_JUMP_IF_ZERO);
  CHECK(ops.items[2].kind == OP_DEC);
  CHECK(ops.items[2].operand == 2);
  CHECK(ops.items[3].kind == OP_JUMP_IF_NONZERO);
  free(ops.items);

  CHECK(parse("+[>-]", &ops));
  CHECK(ops.count == 4);
  CHECK(ops.items[1].kind == OP_JUMP_IF_ZERO);
  free(ops.items);
}

static inline void test_dead_loops_removed(void) {
  ops_t ops;

  CHECK(parse("[-]", &ops));
  CHECK(ops.count == 0);
  free(ops.items);

  CHECK(parse("[>+<-]+", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_INC);
  free(ops.items);

  CHECK(parse("[[-]>[+]]+", &ops));
  CHECK(ops.count == 1);
  CHECK(ops.items[0].kind == OP_INC);
  free(ops.items);

  CHECK(parse("+[>][<]", &ops));
  CHECK(ops.count == 4);
  CHECK(ops.items[0].kind == OP_INC);
  CHECK(ops.items[1].kind == OP_JUMP_IF_ZERO);
  CHECK(ops.items[2].kind == OP_RIGHT);
  CHECK(ops.items[3].kind == OP_JUMP_IF_NONZERO);
  free(ops.items);

  CHECK(parse("+[-][>]", &ops));
  CHECK(ops.count == 2);
  CHECK(ops.items[1].kind == OP_CLEAR);
  free(ops.items);
}

static inline void test_unbalanced_brackets(void) {
  ops_t ops;

  CHECK(!parse("+[", &ops));
  free(ops.items);

  CHECK(!parse("+]", &ops));
  free(ops.items);

  CHECK(!parse("+[>+", &ops));
  free(ops.items);

  CHECK(!parse("+[>[+]", &ops));
  free(ops.items);

  CHECK(!parse("[", &ops));
  free(ops.items);

  CHECK(!parse("[+", &ops));
  free(ops.items);
}

static inline void test_missing_file(void) {
  ops_t ops = {0};
  CHECK(!generate_ops("/tmp/bf_test_does_not_exist_12345", &ops));
  free(ops.items);
}

void run_all_ir_tests(void) {
  printf("[ir]:\n");
  test_empty_and_comments();
  test_run_length_folding();
  test_mixed_ops();
  test_loop_structure();
  test_clear_optimization();
  test_dead_loops_removed();
  test_unbalanced_brackets();
  test_missing_file();
}
