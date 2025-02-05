#include "emit.h"
#include <stdio.h>
#include "ir.h"
#include "parser.h"
#include "typer.h"
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
  lexer_state_populate_lookahead_buffer(state);
  while (1) {
    AST *node = parse_next_statement(arena, state, program);
    if (!node)
      break;
    ast_list_push(&program->statements, node);
  }
}


void *popen(char *, char *);
int pclose(void *);

int main(int argc, char *argv[]) {
  Lexer_State state;
  lexer_state_read_file(&state, "max.it");

  AST_Arena arena = {0};
  AST program;

  // Parsing phase
  TIME_REGION("parsed", { parse_program(&state, &arena, &program); });

  // Typing phase
  TIME_REGION("completed type checking", { type_check_program(program); });

  #define execute_as_c_code false
  if (execute_as_c_code) {
    // Emitting phase
    String_Builder builder;
    char *c_code;
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
  } else {
    IR_Context context;
    AST *entry_point = nullptr;

    for (int i = 0; i < program.statements.length; ++i) {
      AST *statement = program.statements.data[i];
      if (statement->kind == AST_NODE_FUNCTION_DECLARATION && statement->function_declaration.is_entry) {
        entry_point = statement;
        break;
      }
    }

    if (!entry_point) {
      panic("Unable to find entry point. Be sure to mark one of your functions with `fn ...() @entry { ... }` the `@entry` @tribute :D");
    }

    generate_ir(&context, entry_point);
    printf("generated %ld instructions\n", context.length);
  }

  free_lexer_state(&state);
  return 0;
}