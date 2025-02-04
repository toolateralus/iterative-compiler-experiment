#include <stdio.h>
#include "lexer.h"

int main(int argc, char *argv[]) {
  Lexer_State state;
  lexer_state_read_file(&state, "max.it");
  while (1) {
    Token token = token_eat(&state);
    char buffer[1024];
    memset(buffer, 0, 1024);
    memcpy(buffer, token.start, token.length);
    printf("%d :: %s -> %s\n", token.type, Token_Type_Name(token.type), buffer);
    if (token.type == TOKEN_EOF_OR_INVALID) break;
  } 
	return 0;
}