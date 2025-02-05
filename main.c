#include "emit.h"
#include "lexer.h"
#include "parser.h"
#include "type.h"
#include "string_builder.h"
#include "typer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

Type type_table[1024] = {0};
size_t type_table_length = 0;

void emit_program(AST program, String_Builder *builder) {
  string_builder_appendf(builder, "typedef %s %s;\n", "int", "i32");
  string_builder_appendf(builder, "typedef %s %s;\n", "char *", "String");

  // All types first.
  for (int i = 0; i < type_table_length; ++i) {
    if (type_table[i].declaring_node) {
      emit_type_declaration(builder, type_table[i].declaring_node);
    }
  }

  // Then all extern functions
  for (int i = 0; i < program.statements.length; ++i) {
    auto statement = program.statements.data[i];
    if (statement->kind == AST_NODE_FUNCTION_DECLARATION &&
        statement->function_declaration.is_extern)
      emit(builder, statement);
  }

  // Then the rest of the functions.
  for (int i = 0; i < program.statements.length; ++i) {
    auto statement = program.statements.data[i];
    if (statement->kind == AST_NODE_FUNCTION_DECLARATION &&
        !statement->function_declaration.is_entry &&
        !statement->function_declaration.is_extern) {
      emit(builder, statement);
    }
  }

  // Finally the rest.
  for (int i = 0; i < program.statements.length; ++i) {
    emit(builder, program.statements.data[i]);
  }
}

void type_check_program(AST program) {
  initialize_type_system();
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
}

void parse_program(Lexer_State *state, AST_Arena *arena, AST *program) {
  while (1) {
    AST *node = parse_next_statement(arena, state, program);
    if (!node)
      break;
    ast_list_push(&program->statements, node);
  }
}

int main(int argc, char *argv[]) {
  Lexer_State state;
  lexer_state_read_file(&state, "max.it");

  AST_Arena arena = {0};
  AST program;

  clock_t start, end;
  double cpu_time_used;

  // Parsing phase
  start = clock();
  parse_program(&state, &arena, &program);
  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("parsed %ld statements in %f seconds\n", program.statements.length, cpu_time_used);

  // Typing phase
  start = clock();
  type_check_program(program);
  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("completed type checking in %f seconds\n", cpu_time_used);

  // Emitting phase
  {
    String_Builder builder;
    string_builder_init(&builder);

    start = clock();
    emit_program(program, &builder);
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("emitted program in %f seconds\n", cpu_time_used);

    char *c_code = string_builder_get_string(&builder);
    string_builder_free(&builder);

    FILE *file = fopen("generated/output.c", "w");
    int length = strlen(c_code);
    fwrite(c_code, sizeof(char), length, file);
    fclose(file);
  }

  // Compiling phase
  start = clock();
  system("clang -std=c23 -w -g generated/output.c -o generated/output && ./generated/output");
  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("compiled and executed program in %f seconds\n", cpu_time_used);

  free_lexer_state(&state);
  return 0;
}
