#include "utils.h"
#include <stdio.h>

bool read_entire_file(const char* file_path, string_builder_t* sb) {
  FILE* f = fopen(file_path, "rb");
  if (!f) {
    perror("fopen");
    return false;
  }

  if (fseek(f, 0, SEEK_END) < 0) {
    perror("fseek");
    fclose(f);
    return false;
  }
  
  long size = ftell(f);
  if (size < 0) {
    perror("ftell");
    fclose(f);
    return false;
  } else if (size == 0) {
    fclose(f);
    return true;
  }
  
  if (fseek(f, 0, SEEK_SET) < 0) {
    perror("fseek");
    fclose(f);
    return false;
  }

  size_t new_count = sb->count + size;
  if (new_count > sb->cap) {
    void* ptr = realloc(sb->items, new_count);
    if (!ptr) {
      perror("realloc");
      fclose(f);
      return false;
    }
    sb->items = ptr;
    sb->cap = new_count;
  }

  size_t n = fread(sb->items + sb->count, 1, (size_t)size, f);
  if (n != (size_t)size) {
    if (ferror(f))
      perror("fread");
    fclose(f);
    return false;
  }
  sb->count = new_count;
  
  fclose(f);
  return true;
}
