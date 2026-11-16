#ifndef IR_H
#define IR_H

#include "opcode.h"
#include <stdbool.h>

typedef struct {
  size_t* items;
  size_t  count;
  size_t  cap;
} addr_t;

bool generate_ops(const char* file_path, ops_t* ops);

#endif // IR_H
