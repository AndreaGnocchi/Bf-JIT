#include "ir.h"
#include "lexer.h"
#include "opcode.h"
#include "utils.h"
#include <stdbool.h>

bool generate_ops(const char* file_path, ops_t* ops) {
  string_builder_t sb    = {0};
  addr_t stack = {0};

  if (!read_entire_file(file_path, &sb)) return false;

  lexer_t l = { sb, 0};
  char    c = lexer_next(&l);

  while (c) {
    switch (c) {
    case '.': case ',': case '<': case '>': case '-': case '+': {
      size_t count = 1;
      char s = lexer_next(&l);
      while (s == c) {
        count++;
        s = lexer_next(&l);
      }

      op_t op = { c, count};
      DA_APPEND(ops, op);

      c = s;
    } break;
      
    case '[': {
      bool is_fake = (ops->count == 0) ||
        (ops->items[ops->count - 1].kind == OP_JUMP_IF_NONZERO) ||
        (ops->items[ops->count - 1].kind == OP_CLEAR);
      
      if (is_fake) {
        size_t depth = 1;
        while (depth > 0) {
          c = lexer_next(&l);
          if (!c) {
            free(stack.items);
            free(sb.items);
            return false;
          }

          if (c == '[') depth++;
          else if (c == ']') depth--;
        }
        c = lexer_next(&l);
      } else {
        size_t open_idx = ops->count;
        op_t op = { '[', 0 };
        DA_APPEND(ops, op);
        DA_APPEND(&stack, open_idx);

        c = lexer_next(&l);
      }
    } break;

    case ']': {
     if (stack.count == 0) {
        free(stack.items);
        free(sb.items);
        return false;
      }

     size_t open_idx  = stack.items[--stack.count];

     bool is_clear = (ops->count == open_idx + 2) &&
       (ops->items[open_idx + 1].kind == OP_INC ||
        ops->items[open_idx + 1].kind == OP_DEC) &&
       (ops->items[open_idx + 1].operand % 2 == 1);

     if (is_clear) {
       ops->count = open_idx;
       op_t op = { OP_CLEAR, 0};
       DA_APPEND(ops, op);
     } else {
       size_t close_idx = ops->count;
       ops->items[open_idx].operand = close_idx;
       op_t op = { ']', open_idx };
       DA_APPEND(ops, op);
     }
     
     c = lexer_next(&l);
    } break;
      
    default: {}
    }
  }

  bool ok = (stack.count == 0);
  free(stack.items);
  free(sb.items);
  return ok;
}
