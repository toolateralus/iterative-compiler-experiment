#include "emit.h"
#include "lexer.h"
#include "parser.h"
#include "string_builder.h"
#include "type.h"
#include "typer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define PRINT_COLOR "\033[1;33m"
#define RESET_COLOR "\033[0m"
#define RUN_COLOR "\033[1;32m"
#define TIME_COLOR "\033[1;34m"

#define TIME_DIFF(start, end) ((double)(end - start) / CLOCKS_PER_SEC)

#define PRINT_TIME(label, time_sec)                                            \
  do {                                                                         \
    if (time_sec < 1e-6)                                                       \
      printf("%s%s in %s%.2f ns%s\n", PRINT_COLOR, label, TIME_COLOR,          \
             time_sec * 1e9, RESET_COLOR);                                     \
    else if (time_sec < 1e-3)                                                  \
      printf("%s%s in %s%.2f µs%s\n", PRINT_COLOR, label, TIME_COLOR,          \
             time_sec * 1e6, RESET_COLOR);                                     \
    else if (time_sec < 1)                                                     \
      printf("%s%s in %s%.2f ms%s\n", PRINT_COLOR, label, TIME_COLOR,          \
             time_sec * 1e3, RESET_COLOR);                                     \
    else                                                                       \
      printf("%s%s in %s%.2f s%s\n", PRINT_COLOR, label, TIME_COLOR, time_sec, \
             RESET_COLOR);                                                     \
  } while (0)

#define TIME_REGION(label, code)                                               \
  do {                                                                         \
    clock_t start = clock();                                                   \
    code clock_t end = clock();                                                \
    double time_sec = TIME_DIFF(start, end);                                   \
    PRINT_TIME(label, time_sec);                                               \
  } while (0)


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

#include <stdio.h>

void *popen(char*, char*);
int pclose(void*);

int main(int argc, char *argv[]) {
  Lexer_State state;
  lexer_state_read_file(&state, "max.it");

  AST_Arena arena = {0};
  AST program;

  // Parsing phase
  TIME_REGION("parsed", { parse_program(&state, &arena, &program); });

  // Typing phase
  TIME_REGION("completed type checking", { type_check_program(program); });

  // Emitting phase
  String_Builder builder;
  char* c_code;
  TIME_REGION("emitted program", {
    string_builder_init(&builder);
    emit_program(program, &builder);
    c_code = string_builder_get_string(&builder);
    string_builder_free(&builder);
  });

  // Compiling phase
  TIME_REGION("write file, compile c code.", {
    FILE *clang = popen("clang -std=c23 -w -x c - -o generated/output", "w");
    if (clang == NULL) {
      perror("popen");
      exit(EXIT_FAILURE);
    }
    fprintf(clang, "%s", c_code);
    if (pclose(clang) == -1) {
      perror("pclose");
      exit(EXIT_FAILURE);
    }
  });

  printf("%srunning './generated/output' binary...%s\n", RUN_COLOR,
         RESET_COLOR);
  TIME_REGION("executed program", { system("./generated/output"); });

  free_lexer_state(&state);
  return 0;
}