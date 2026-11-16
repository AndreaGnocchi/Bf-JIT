#include "ir.h"
#include "jit.h"
#include "opcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define MAX_MEMORY (10 << 20)

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

  void* memory = malloc(MAX_MEMORY);
  if (!memory) {
    fprintf(stderr, "Error: could not allocate %d bytes for the tape\n", MAX_MEMORY);
    free(ops.items);
    munmap(code.run, code.len);
    return 1;
  }
  memset(memory, 0, MAX_MEMORY);
  code.run(memory);
  
  free(ops.items);
  munmap(code.run, code.len);
  free(memory);
  return 0;
}
