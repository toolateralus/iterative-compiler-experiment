#include "lexer.h"
#include <stdio.h>
#include "type.h"
#include "parser.h"

Type type_table[1024] = {0};
size_t type_table_length = 0;

int main(int argc, char *argv[]) {
  Lexer_State state;
  lexer_state_read_file(&state, "max.it");
  while (1) {
    Token token = token_eat(&state);
    char buffer[1024];
    memset(buffer, 0, 1024);
    memcpy(buffer, token.value.start, token.value.length);
    printf("%-5d :: %-25s -> '%s'\n", token.type, Token_Type_Name(token.type), buffer);
    if (token.type == TOKEN_EOF_OR_INVALID) {
      break;
    }
  }
  return 0;
}