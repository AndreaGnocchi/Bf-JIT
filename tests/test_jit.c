#include "test_main.h"
#include "../src/jit.h"
#include "../src/opcode.h"
#include <stdint.h>
#include <sys/mman.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

#define OPS(arr) ((ops_t){ (arr), ARRAY_LEN(arr), ARRAY_LEN(arr) })

static inline void free_code(code_t* code) {
  if (code->run) munmap((void*)code->run, code->len);
  code->run = NULL;
  code->len = 0;
}

static inline bool run_ops(ops_t ops, uint8_t* tape) {
  code_t code = {0};
  if (!jit_compile(ops, &code)) return false;
  code.run(tape);
  free_code(&code);
  return true;
}

static inline size_t run_capture_stdout(ops_t ops, uint8_t* tape, char* buf, size_t buf_size) {
  code_t code = {0};
  if (!jit_compile(ops, &code)) return 0;

  int fds[2];
  if (pipe(fds) < 0) { free_code(&code); return 0; }

  fflush(stdout);
  int saved = dup(1);
  dup2(fds[1], 1);

  code.run(tape);

  dup2(saved, 1);
  close(saved);
  close(fds[1]);

  ssize_t n = read(fds[0], buf, buf_size);
  close(fds[0]);
  free_code(&code);
  return n < 0 ? 0 : (size_t)n;
}

static inline bool run_with_stdin(ops_t ops, uint8_t* tape, const char* input) {
  code_t code = {0};
  if (!jit_compile(ops, &code)) return false;

  int fds[2];
  if (pipe(fds) < 0) { free_code(&code); return false; }

  size_t len = strlen(input);
  if (write(fds[1], input, len) != (ssize_t)len) {
    close(fds[0]); close(fds[1]); free_code(&code);
    return false;
  }
  close(fds[1]);

  int saved = dup(0);
  dup2(fds[0], 0);

  code.run(tape);

  dup2(saved, 0);
  close(saved);
  close(fds[0]);
  free_code(&code);
  return true;
}

static inline void test_compile_output(void) {
  op_t prog[] = { { OP_INC, 1 } };
  code_t code = {0};

  CHECK(jit_compile(OPS(prog), &code));
  CHECK(code.run != NULL);
  CHECK(code.len > 0);
  free_code(&code);

  ops_t empty = {0};
  CHECK(jit_compile(empty, &code));
  CHECK(code.run != NULL);
  CHECK(code.len == 6);
  uint8_t tape[4] = {1, 2, 3, 4};
  code.run(tape);
  CHECK(tape[0] == 1 && tape[1] == 2 && tape[2] == 3 && tape[3] == 4);
  free_code(&code);
}

static inline void test_inc_dec(void) {
  uint8_t tape[8] = {0};

  op_t inc[] = { { OP_INC, 5 } };
  CHECK(run_ops(OPS(inc), tape));
  CHECK(tape[0] == 5);

  op_t dec[] = { { OP_DEC, 2 } };
  CHECK(run_ops(OPS(dec), tape));
  CHECK(tape[0] == 3);

  uint8_t zero[1] = {0};
  CHECK(run_ops(OPS(dec), zero));
  CHECK(zero[0] == 254);

  op_t wrap[] = { { OP_INC, 300 } };
  uint8_t t2[1] = {0};
  CHECK(run_ops(OPS(wrap), t2));
  CHECK(t2[0] == 44);

  op_t full[] = { { OP_INC, 256 } };
  uint8_t t3[1] = {9};
  CHECK(run_ops(OPS(full), t3));
  CHECK(t3[0] == 9);

  CHECK(tape[1] == 0 && tape[7] == 0);
}

static inline void test_pointer_moves(void) {
  uint8_t tape[16] = {0};

  op_t right[] = { { OP_RIGHT, 3 }, { OP_INC, 7 } };
  CHECK(run_ops(OPS(right), tape));
  CHECK(tape[3] == 7);
  CHECK(tape[0] == 0);

  op_t roundtrip[] = { { OP_RIGHT, 10 }, { OP_LEFT, 10 }, { OP_INC, 2 } };
  uint8_t t2[16] = {0};
  CHECK(run_ops(OPS(roundtrip), t2));
  CHECK(t2[0] == 2);

  static uint8_t big[1024] = {0};
  op_t far[] = { { OP_RIGHT, 1000 }, { OP_INC, 1 } };
  CHECK(run_ops(OPS(far), big));
  CHECK(big[1000] == 1);
}

static inline void test_clear(void) {
  uint8_t tape[4] = {0xAB, 0xCD, 0, 0};
  op_t prog[] = { { OP_CLEAR, 0 } };
  CHECK(run_ops(OPS(prog), tape));
  CHECK(tape[0] == 0);
  CHECK(tape[1] == 0xCD);
}

static inline void test_loops(void) {
  op_t mul[] = {
    { OP_INC,             3 },
    { OP_JUMP_IF_ZERO,    6 },
    { OP_RIGHT,           1 },
    { OP_INC,             2 },
    { OP_LEFT,            1 },
    { OP_DEC,             1 },
    { OP_JUMP_IF_NONZERO, 1 },
  };
  uint8_t tape[4] = {0};
  CHECK(run_ops(OPS(mul), tape));
  CHECK(tape[0] == 0);
  CHECK(tape[1] == 6);

  op_t skip[] = {
    { OP_JUMP_IF_ZERO,    3 },
    { OP_RIGHT,           1 },
    { OP_INC,             5 },
    { OP_JUMP_IF_NONZERO, 0 },
  };
  uint8_t t2[4] = {0};
  CHECK(run_ops(OPS(skip), t2));
  CHECK(t2[0] == 0 && t2[1] == 0);

  op_t nested[] = {
    { OP_INC,             2 },
    { OP_JUMP_IF_ZERO,    12 },
    { OP_RIGHT,           1 },
    { OP_INC,             3 },
    { OP_JUMP_IF_ZERO,    9 },
    { OP_RIGHT,           1 },
    { OP_INC,             1 },
    { OP_LEFT,            1 },
    { OP_DEC,             1 },
    { OP_JUMP_IF_NONZERO, 4 },
    { OP_LEFT,            1 },
    { OP_DEC,             1 },
    { OP_JUMP_IF_NONZERO, 1 },
  };
  uint8_t t3[4] = {0};
  CHECK(run_ops(OPS(nested), t3));
  CHECK(t3[0] == 0);
  CHECK(t3[1] == 0);
  CHECK(t3[2] == 6);
}

static inline void test_output(void) {
  char buf[16] = {0};

  uint8_t tape[2] = {'A', 0};
  op_t one[] = { { OP_OUTPUT, 1 } };
  size_t n = run_capture_stdout(OPS(one), tape, buf, sizeof(buf));
  CHECK(n == 1);
  CHECK(buf[0] == 'A');

  op_t three[] = { { OP_OUTPUT, 3 } };
  memset(buf, 0, sizeof(buf));
  n = run_capture_stdout(OPS(three), tape, buf, sizeof(buf));
  CHECK(n == 3);
  CHECK(memcmp(buf, "AAA", 3) == 0);

  op_t calc[] = {
    { OP_INC,             8 },
    { OP_JUMP_IF_ZERO,    6 },
    { OP_RIGHT,           1 },
    { OP_INC,             8 },
    { OP_LEFT,            1 },
    { OP_DEC,             1 },
    { OP_JUMP_IF_NONZERO, 1 },
    { OP_RIGHT,           1 },
    { OP_INC,             1 },
    { OP_OUTPUT,          1 },
  };
  uint8_t t2[4] = {0};
  memset(buf, 0, sizeof(buf));
  n = run_capture_stdout(OPS(calc), t2, buf, sizeof(buf));
  CHECK(n == 1);
  CHECK(buf[0] == 'A');
}

static inline void test_input(void) {
  uint8_t tape[4] = {0};

  op_t one[] = { { OP_INPUT, 1 } };
  CHECK(run_with_stdin(OPS(one), tape, "Z"));
  CHECK(tape[0] == 'Z');

  op_t two[] = { { OP_INPUT, 2 } };
  uint8_t t2[4] = {0};
  CHECK(run_with_stdin(OPS(two), t2, "xy"));
  CHECK(t2[0] == 'y');

  op_t second[] = { { OP_RIGHT, 1 }, { OP_INPUT, 1 } };
  uint8_t t3[4] = {0};
  CHECK(run_with_stdin(OPS(second), t3, "Q"));
  CHECK(t3[0] == 0);
  CHECK(t3[1] == 'Q');
}

static inline void test_echo_program(void) {
  op_t echo[] = {
    { OP_INPUT,           1 },
    { OP_JUMP_IF_ZERO,    4 },
    { OP_OUTPUT,          1 },
    { OP_INPUT,           1 },
    { OP_JUMP_IF_NONZERO, 1 },
  };

  code_t code = {0};
  CHECK(jit_compile(OPS(echo), &code));
  if (!code.run) return;

  int in[2], out[2];
  CHECK(pipe(in) == 0);
  CHECK(pipe(out) == 0);

  const char msg[] = "hi!\0";
  CHECK(write(in[1], msg, sizeof(msg)) == (ssize_t)sizeof(msg));
  close(in[1]);

  fflush(stdout);
  int saved_in  = dup(0);
  int saved_out = dup(1);
  dup2(in[0], 0);
  dup2(out[1], 1);

  uint8_t tape[4] = {0};
  code.run(tape);

  dup2(saved_in, 0);
  dup2(saved_out, 1);
  close(saved_in); close(saved_out);
  close(in[0]); close(out[1]);

  char buf[16] = {0};
  ssize_t n = read(out[0], buf, sizeof(buf));
  close(out[0]);

  CHECK(n == 3);
  CHECK(memcmp(buf, "hi!", 3) == 0);
  free_code(&code);
}

static inline void test_bounds_checking(void) {
  code_t code = {0};

  op_t left[] = { { OP_LEFT, MAX_MEMORY / 2 } };
  CHECK(!jit_compile(OPS(left), &code));

  op_t dec[] = { { OP_DEC, MAX_MEMORY / 2 } };
  CHECK(!jit_compile(OPS(dec), &code));

  op_t left_ok[] = { { OP_LEFT, MAX_MEMORY / 2 - 1 } };
  CHECK(jit_compile(OPS(left_ok), &code));
  free_code(&code);

  op_t dec_ok[] = { { OP_DEC, MAX_MEMORY / 2 - 1 } };
  CHECK(jit_compile(OPS(dec_ok), &code));
  free_code(&code);

  op_t right[] = { { OP_RIGHT, MAX_MEMORY / 2 } };
  CHECK(!jit_compile(OPS(right), &code));

  op_t inc[] = { { OP_INC, MAX_MEMORY / 2 } };
  CHECK(!jit_compile(OPS(inc), &code));

  op_t right_ok[] = { { OP_RIGHT, MAX_MEMORY / 2 - 1 } };
  CHECK(jit_compile(OPS(right_ok), &code));
  free_code(&code);

  op_t inc_ok[] = { { OP_INC, MAX_MEMORY / 2 - 1 } };
  CHECK(jit_compile(OPS(inc_ok), &code));
  free_code(&code);
}

void run_all_jit_tests(void) {
  printf("[jit]:\n");
  test_compile_output();
  test_inc_dec();
  test_pointer_moves();
  test_clear();
  test_loops();
  test_output();
  test_input();
  test_echo_program();
  test_bounds_checking();
}
