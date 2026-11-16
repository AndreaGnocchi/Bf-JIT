#include "lexer.h"

bool is_bf_cmd(char c) {
  const char* cmds = "+-<>,.[]";
  return c != '\0' && strchr(cmds, c) != NULL;
}

char lexer_next(lexer_t* l) {
  while (l->pos < l->content.count && !is_bf_cmd(l->content.items[l->pos])) {
    l->pos++;
  }
  
  if (l->pos >= l->content.count) return 0;
  return l->content.items[l->pos++];
}
