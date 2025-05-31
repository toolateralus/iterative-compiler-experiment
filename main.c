#include "backend.h"
#include "core.h"
#include "graph.h"
#include "parser.h"
#include "thir.h"
#include "type.h"
#include "typer.h"
#include <stdio.h>
#include <time.h>

size_t address = 0;
Vector type_table;
Compilation_Mode COMPILATION_MODE = CM_DEBUG;
int node_printer_indentation = 0;

Arena thir_arena;


void parse_program(Lexer_State *state, AST_Arena *arena, AST *program) {
  lexer_state_populate_lookahead_buffer(state);
  while (1) {
    AST *node = parse_next_statement(arena, state, program);
    if (!node) break;
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

  

  DepNodeRegistry registry = {0};
  DepGraph graph = {0};
  TIME_REGION("create dependency graph", { 
    populate_dep_graph(&registry, &graph, &program);
  });


  if (1) {
    printf("dependency graph:\n");
    print_graph(&graph);
  }


  Vector thir_symbols;
  arena_init(&thir_arena);
  vector_init(&thir_symbols, sizeof(THIRSymbol));
  initialize_type_system();

  THIR *thir;
  TIME_REGION("generating THIR", {
    thir = generate_thir(&graph, &registry, &thir_symbols);
  });

  if (0) {
    printf("thir:\n\033[0;34m");
    pretty_print_thir(thir, 0);
    printf("\033[0m");
  }

  TIME_REGION("generated LLVM IR", {
    LLVM_Emit_Context ctx;
    emit_thir_program(&ctx, thir);
  });
  
  TIME_REGION("compiled LLVM IR", { system("clang -g -lc generated/output.ll -o generated/output"); });

  TIME_REGION("executed 'generated/output' binary", { system("./generated/output"); });
  return 0;
  


  free_lexer_state(&state);
  return 0;
}
