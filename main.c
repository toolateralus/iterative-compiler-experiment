#include "backend.h"
#include "core.h"
#include "parser.h"
#include "typer.h"
#include <stdio.h>
#include <time.h>

size_t address = 0;
Vector type_table;

Compilation_Mode COMPILATION_MODE = CM_DEBUG;

void type_check_program(AST program) {
  initialize_type_system();

  // Find an entry point marked function.
  { 
    AST *entry_point = nullptr;
    for (int i = 0; i < program.statements.length; ++i) {
      AST *statement = program.statements.data[i];
      if (statement->kind == AST_NODE_FUNCTION_DECLARATION &&
          statement->function.is_entry) {
        entry_point = statement;
        break;
      }
    }

    if (!entry_point) {
      panic("Unable to find entry point. Be sure to mark one of your functions "
            "with `fn ...() @entry { ... }` the `@entry` @tribute :D");
    }
  }

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


int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strncmp(argv[1], "-r", 2) == 0) {
      COMPILATION_MODE = CM_RELEASE;
    }
  }
  
  Lexer_State state;
  lexer_state_read_file(&state, "max.it");
  AST_Arena arena = {0};
  AST program;

  TIME_REGION("parsed", { parse_program(&state, &arena, &program); });
  TIME_REGION("completed type checking", { type_check_program(program); });

  TIME_REGION("generated LLVM IR", {
    LLVM_Emit_Context ctx;
    emit_program(&ctx, &program);
  });

  TIME_REGION("compiled LLVM IR", {
    system("clang -g -lc generated/output.ll -o generated/output");
  });

  TIME_REGION("executed 'generated/output' binary", {
    system("./generated/output");
  });

  free_lexer_state(&state);
  return 0;
}