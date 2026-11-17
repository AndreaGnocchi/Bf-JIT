#ifndef JIT_H
#define JIT_H

#include "opcode.h"
#include <stdbool.h>

#define MAX_MEMORY (10 << 20)

typedef struct {
    size_t operand_byte_addr;
    size_t src_byte_addr;
    size_t dst_op_index;
} backpatch_t;

typedef struct {
    backpatch_t* items;
    size_t       count;
    size_t       cap;
} backpatches_t;

typedef struct {
    void (*run)(void *memory);
    size_t len;
} code_t;

bool jit_compile(ops_t ops, code_t* code);

#endif // JIT_H
