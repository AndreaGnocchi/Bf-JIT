#ifndef LEXER_H
#define LEXER_H

#include "utils.h"

typedef struct {
  string_builder_t content;
  size_t           pos;
} lexer_t;

bool is_bf_cmd(char c);
char lexer_next(lexer_t* l);

#endif // LEXER_H
