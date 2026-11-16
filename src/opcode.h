#ifndef OPCODE_H
#define OPCODE_H

#include <stddef.h>

typedef enum {
  OP_INC             = '+',
  OP_DEC             = '-',
  OP_LEFT            = '<',
  OP_RIGHT           = '>',
  OP_OUTPUT          = '.',
  OP_INPUT           = ',',
  OP_JUMP_IF_ZERO    = '[',
  OP_JUMP_IF_NONZERO = ']',
} op_kind_t;

typedef struct {
  op_kind_t kind;
  size_t    operand;
} op_t;

typedef struct {
  op_t*  items;
  size_t count;
  size_t cap;
} ops_t;

#endif // OPCODE_H
