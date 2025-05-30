
#ifndef TYPER2_H
#define TYPER2_H

#include "graph.h"
#include "thir.h"

bool dep_node_dependencies_resolved(DepNode *node);
THIR *generate_thir_from_ast(AST *node, Vector *thir_symbols);
void generate_thir_for_node(DepNode *node, Vector *thir_symbols, THIR *program);
THIR *generate_thir(DepGraph *graph, DepNodeRegistry *registry, Vector *thir_symbols);

#endif