#include "test_main.h"
#include "../src/utils.h"

static inline void test_da_append(void) {
  string_builder_t sb = {0};

  DA_APPEND(&sb, 'a');
  DA_APPEND(&sb, 'b');
  CHECK(sb.count == 2);
  CHECK(sb.items[0] == 'a');
  CHECK(sb.items[1] == 'b');
  CHECK(sb.cap >= 2);

  for (int i = 0; i < 1000; ++i) DA_APPEND(&sb, 'x');
  CHECK(sb.count == 1002);
  CHECK(sb.items[0] == 'a');
  CHECK(sb.items[1001] == 'x');

  free(sb.items);
}

static inline void test_da_append_many(void) {
  string_builder_t sb = {0};

  DA_APPEND_MANY(&sb, "hello", 5);
  CHECK(sb.count == 5);
  CHECK(memcmp(sb.items, "hello", 5) == 0);

  char big[500];
  memset(big, 'z', sizeof(big));
  DA_APPEND_MANY(&sb, big, sizeof(big));
  CHECK(sb.count == 505);
  CHECK(sb.cap >= 505);
  CHECK(memcmp(sb.items, "hello", 5) == 0);
  CHECK(sb.items[504] == 'z');

  free(sb.items);
}

static inline void test_sb_append_cstr(void) {
  string_builder_t sb = {0};

  SB_APPEND_CSTR(&sb, "abc");
  SB_APPEND_CSTR(&sb, "def");
  CHECK(sb.count == 6);
  CHECK(memcmp(sb.items, "abcdef", 6) == 0);

  SB_APPEND_CSTR(&sb, "\x53\x48\x89\xfb");
  CHECK(sb.count == 10);
  CHECK((unsigned char)sb.items[9] == 0xfb);

  free(sb.items);
}

static void test_read_entire_file(void) {
  char path[64];

  CHECK(write_temp_file("+-<>[],.", path, sizeof(path)));
  string_builder_t sb = {0};
  CHECK(read_entire_file(path, &sb));
  CHECK(sb.count == 8);
  CHECK(memcmp(sb.items, "+-<>[],.", 8) == 0);
  free(sb.items);
  unlink(path);

  CHECK(write_temp_file("", path, sizeof(path)));
  string_builder_t empty = {0};
  CHECK(read_entire_file(path, &empty));
  CHECK(empty.count == 0);
  free(empty.items);
  unlink(path);

  CHECK(write_temp_file("world", path, sizeof(path)));
  string_builder_t app = {0};
  SB_APPEND_CSTR(&app, "hello ");
  CHECK(read_entire_file(path, &app));
  CHECK(app.count == 11);
  CHECK(memcmp(app.items, "hello world", 11) == 0);
  free(app.items);
  unlink(path);

  string_builder_t missing = {0};
  CHECK(!read_entire_file("/tmp/bf_test_does_not_exist_12345", &missing));
  CHECK(missing.count == 0);
  free(missing.items);
}

void run_all_utils_tests(void) {
  printf("[utils]:\n");
  test_da_append();
  test_da_append_many();
  test_sb_append_cstr();
  test_read_entire_file();
}
