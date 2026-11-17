#include "ir.h"
#include "jit.h"
#include "opcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Not enough arguments. Usage: %s <file>\n", argv[0]);
    return 1;
  } else if (argc > 2) {
    fprintf(stderr, "Too many arguments: expected 1 file, got %d\n", argc - 1);
    return 1;
  }

  const char* file_path = argv[1];
  ops_t       ops       = {0};
  code_t      code      = {0};
  if (!generate_ops(file_path, &ops)) {
    fprintf(stderr, "Error: could not read or parse '%s' (unmatched bracket or unreadable file)\n", file_path);
    free(ops.items);
    return 1;
  }

  if (!jit_compile(ops, &code)) {
    fprintf(stderr, "Error: JIT compilation failed\n");
    free(ops.items);
    return 1;
  }

  long   ps         = sysconf(_SC_PAGESIZE);
  size_t total_size = MAX_MEMORY + (8 * ps);

  void* base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (base == MAP_FAILED) {
    perror("mmap failed");
    free(ops.items);
    munmap(code.run, code.len);
    return 1;
  }

  if (mprotect(base, 4 * ps, PROT_NONE) == -1) {
    perror("mprotect guard 1 failed");
    munmap(base, total_size);
    free(ops.items);
    munmap(code.run, code.len);
    return 1;
  }
  
  void* end_guard = (char*)base + 4 * ps + MAX_MEMORY;
  if (mprotect(end_guard, 4 * ps, PROT_NONE) == -1) {
    perror("mprotect guard 2 failed");
    munmap(base, total_size);
    free(ops.items);
    munmap(code.run, code.len);
    return 1;
  }
  
  void* memory = (char*)base + 4 * ps;
  memset(memory, 0, MAX_MEMORY);
  
  code.run((char*)memory + MAX_MEMORY / 2);

  munmap(base, total_size);  
  free(ops.items);
  munmap(code.run, code.len);
  return 0;
}
