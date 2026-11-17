#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern size_t test_count;
extern size_t test_passed;

#define CHECK(cond) do {                                                       \
  test_count++;                                                                \
  if (cond) {                                                                  \
      test_passed++;                                                           \
  } else {                                                                     \
      fprintf(stderr, "%s:%d: CHECK FAILED: %s\n", __FILE__, __LINE__, #cond); \
  }                                                                            \
} while (0)

static inline bool write_temp_file(const char* content, char* path, size_t path_size) {
  const char* tmp = "/tmp/bf_test_XXXXXX";
  if (path_size < strlen(tmp) + 1) return false;
  strcpy(path, tmp);

  int fd = mkstemp(path);
  if (fd < 0) return false;

  size_t len = strlen(content);
  bool   ok  = (write(fd, content, len) == (ssize_t)len);
  close(fd);
  if (!ok)
    unlink(path);

  return ok;
}

#endif // TEST_MAIN_H
