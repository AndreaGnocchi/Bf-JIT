#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static inline void* _realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr && size != 0) {
        fprintf(stderr, "Fatal: Out of memory (requested %zu bytes)\n", size);
        abort();
    }
    return new_ptr;
}

#define INIT_CAP 128

#define DA_APPEND(da, item)                                                    \
  do {                                                                         \
    if ((da)->count >= (da)->cap) {                                             \
      size_t new_cap = (da)->cap == 0 ? INIT_CAP : (da)->cap * 2;              \
      (da)->items = _realloc((da)->items, new_cap * sizeof(*(da)->items));     \
      (da)->cap = new_cap;                                                     \
    }                                                                          \
                                                                               \
    (da)->items[(da)->count++] = (item);                                       \
  } while (0)

#define DA_APPEND_MANY(da, new_items, new_items_count)                                    \
  do {                                                                                    \
    if ((da)->count + new_items_count > (da)->cap) {                                      \
      if ((da)->cap == 0) {                                                               \
        (da)->cap = INIT_CAP;                                                             \
      }                                                                                   \
      while ((da)->count + new_items_count > (da)->cap) {                                 \
        (da)->cap *= 2;                                                                   \
      }                                                                                   \
      (da)->items = _realloc((da)->items, (da)->cap * sizeof(*(da)->items));              \
    }                                                                                     \
    memcpy((da)->items + (da)->count, new_items, new_items_count * sizeof(*(da)->items)); \
    (da)->count += new_items_count;                                                       \
  } while (0)

typedef struct {
  char* items;
  size_t cap;
  size_t count;
} string_builder_t;

#define SB_APPEND_CSTR(sb, cstr)                \
  do {                                          \
    const char* s = (cstr);                     \
    size_t      l = strlen(s);                  \
    DA_APPEND_MANY(sb, s, l);                   \
  } while (0)

bool read_entire_file(const char* file_path, string_builder_t* sb);

#endif // UTILS_H
