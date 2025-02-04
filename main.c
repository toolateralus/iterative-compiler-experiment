#include "lexer.h"
#include "parser.h"
#include "type.h"
#include <stdio.h>

Type type_table[1024] = {0};
size_t type_table_length = 0;

void debug_print_all_tokens(Lexer_State *state) {
  while (1) {
    Token token = token_eat(state);
    char buffer[1024];
    memset(buffer, 0, 1024);
    memcpy(buffer, token.value.start, token.value.length);
    printf("%-5d :: %-25s -> '%s'\n", token.type, Token_Type_Name(token.type),
           buffer);
    if (token.type == TOKEN_EOF_OR_INVALID) {
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  Lexer_State state;
  lexer_state_read_file(&state, "max.it");


  free_lexer_state(&state);
  return 0;
}