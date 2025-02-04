#include "emit.h"
#include "lexer.h"
#include "parser.h"
#include "string_builder.h"
#include "type.h"
#include "typer.h"
#include <stdio.h>
#include <stdlib.h>

Type type_table[1024] = {0};
size_t type_table_length = 0;

void debug_print_all_tokens(Lexer_State *state) {
  while (1) {
    Token token = token_eat(state);
    char buffer[1024];
    memset(buffer, 0, 1024);
    memcpy(buffer, token.value.data, token.value.length);
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
  AST_Arena arena = {0};
  AST program;
  initialize_type_system();
  while (1) {
    AST *node = parse_next_statement(&arena, &state, &program);
    if (!node)
      break;
    ast_list_push(&program.statements, node);
  }

  printf("parsed %ld statements\n", program.statements.length);

  while (1) {
    bool done = true;
    for (int i = 0; i < program.statements.length; ++i) {
      if (typer_resolve(program.statements.data[i]) == UNRESOLVED) {
        done = false;
      }
    }
    if (done)
      break;
  }

  printf("completed type checking\n");

  String_Builder builder;
  string_builder_init(&builder);

  string_builder_appendf(&builder, "typedef %s %s;\n", "int", "i32");
  string_builder_appendf(&builder, "typedef %s %s;\n", "char *", "String");

  for (int i = 0; i < type_table_length; ++i) {
    if (type_table[i].declaring_node) {
      emit_type_declaration(&builder, type_table[i].declaring_node);
    }
  }

  for (int i = 0; i < program.statements.length; ++i) {
    if (program.statements.data[i]->kind == AST_NODE_FUNCTION_DECLARATION &&
        !program.statements.data[i]->function_declaration.is_entry)
      emit(&builder, program.statements.data[i]);
  }

  for (int i = 0; i < program.statements.length; ++i) {
    emit(&builder, program.statements.data[i]);
  }


  FILE *file = fopen("output.c", "w");
  char *c_code = string_builder_get_string(&builder);
  int length = strlen(c_code);
  fwrite(c_code, sizeof(char), length, file);
  fclose(file);

  system("clang -std=c23 -w -g output.c -o output && ./output");

  free_lexer_state(&state);
  return 0;
}