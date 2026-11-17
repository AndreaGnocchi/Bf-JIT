#include "test_main.h"
#include "../src/lexer.h"

static inline lexer_t make_lexer(const char* src) {
  string_builder_t sb = { (char*)src, strlen(src), strlen(src) };
  lexer_t l = { sb, 0 };
  return l;
}

static inline void test_is_bf_cmd(void) {
  const char* cmds = "+-<>,.[]";
  for (const char* p = cmds; *p; ++p) CHECK(is_bf_cmd(*p));

  CHECK(!is_bf_cmd('a'));
  CHECK(!is_bf_cmd('Z'));
  CHECK(!is_bf_cmd('0'));
  CHECK(!is_bf_cmd(' '));
  CHECK(!is_bf_cmd('\n'));
  CHECK(!is_bf_cmd('\0'));
}

static inline void test_lexer_next_basic(void) {
  lexer_t l = make_lexer("+-<>,.[]");
  CHECK(lexer_next(&l) == '+');
  CHECK(lexer_next(&l) == '-');
  CHECK(lexer_next(&l) == '<');
  CHECK(lexer_next(&l) == '>');
  CHECK(lexer_next(&l) == ',');
  CHECK(lexer_next(&l) == '.');
  CHECK(lexer_next(&l) == '[');
  CHECK(lexer_next(&l) == ']');
  CHECK(lexer_next(&l) == 0);
  CHECK(lexer_next(&l) == 0);
}

static inline void test_lexer_next_skips_comments(void) {
  lexer_t l = make_lexer("hello + world\n- foo [ bar ] 123");
  CHECK(lexer_next(&l) == '+');
  CHECK(lexer_next(&l) == '-');
  CHECK(lexer_next(&l) == '[');
  CHECK(lexer_next(&l) == ']');
  CHECK(lexer_next(&l) == 0);
}

static inline void test_lexer_next_empty(void) {
  lexer_t l = make_lexer("");
  CHECK(lexer_next(&l) == 0);

  lexer_t only_comments = make_lexer("no commands here\n");
  CHECK(lexer_next(&only_comments) == 0);
}

static inline void test_lexer_position(void) {
  lexer_t l = make_lexer("ab+cd");
  CHECK(l.pos == 0);
  CHECK(lexer_next(&l) == '+');
  CHECK(l.pos == 3);
  CHECK(lexer_next(&l) == 0);
  CHECK(l.pos == 5);
}

void run_all_lexer_tests(void) {
  printf("[lexer]:\n");
  test_is_bf_cmd();
  test_lexer_next_basic();
  test_lexer_next_skips_comments();
  test_lexer_next_empty();
  test_lexer_position();
}
