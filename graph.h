#ifndef GRAPH_H
#define GRAPH_H

#include "parser.h"
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  UNRESOLVED=0,
  RESOLVING,
  RESOLVED,
  ERRORED,
} DepState;

typedef struct DepNode {
  AST *ast_node;

  struct DepNode **dependencies;
  size_t length;
  size_t capacity;

  char *error;
  DepState state;
} DepNode;

typedef struct DepGraph {
  DepNode **nodes;
  size_t length;
  size_t capacity;
} DepGraph;

typedef struct DepNodeRegistry {
  DepNode **nodes;
  size_t length;
  size_t capacity;
} DepNodeRegistry;

static inline void add_node_to_dep_registry(DepNodeRegistry *registry, DepNode *node);

static inline DepNode *create_dep_node(AST *node, DepNodeRegistry *registry) {
  // caching/deduplication
  for (int i = 0; i < registry->length; ++i) {
    DepNode *dep_node = registry->nodes[i];
    if (dep_node->ast_node == node) {
      return dep_node;
    }
  }
  DepNode *dep_node = malloc(sizeof(DepNode));
  memset(dep_node, 0, sizeof(DepNode));
  dep_node->ast_node = node;
  add_node_to_dep_registry(registry, dep_node);
  return dep_node;
}

static inline void free_dep_node(DepNode *node) {
  for (int i = 0; i < node->length; ++i) {
    free_dep_node(node->dependencies[i]);
  }
  if (node->error) {
    free(node->error);
  }
  node->dependencies = nullptr;
}

static inline DepGraph *create_dep_graph() {
  DepGraph *graph = malloc(sizeof(DepGraph));
  memset(graph, 0, sizeof(DepGraph));
  return graph;
}

static inline void free_dep_graph(DepGraph *graph) {
  for (int i = 0; i < graph->length; ++i) {
    free_dep_node(graph->nodes[i]);
  }

  graph->length = 0;
  graph->capacity = 0;

  if (graph->nodes) free(graph->nodes);
}

static inline void add_node_to_dep_graph(DepGraph *graph, DepNode *node) {
  if (graph->nodes == nullptr) {
    graph->capacity = 32;
    graph->nodes = realloc(graph->nodes, graph->capacity * sizeof(DepNode *));
  }

  if (graph->length >= graph->capacity) {
    graph->capacity *= 1.5;
    graph->nodes = realloc(graph->nodes, graph->capacity * sizeof(DepNode *));
  }

  graph->nodes[graph->length++] = node;
}

static inline void add_node_to_dep_registry(DepNodeRegistry *registry, DepNode *node) {
  if (registry->nodes == nullptr) {
    registry->capacity = 32;
    registry->nodes = realloc(registry->nodes, registry->capacity * sizeof(DepNode *));
  }

  if (registry->length >= registry->capacity) {
    registry->capacity *= 1.5;
    registry->nodes = realloc(registry->nodes, registry->capacity * sizeof(DepNode *));
  }

  registry->nodes[registry->length++] = node;
}

static inline void add_dep_to_dep_node(DepNode *node, DepNode *dep) {
  if (dep == node) {
    return;
  }
  for (size_t i = 0; i < node->length; ++i) {
    if (node->dependencies[i] == dep) {
      return;
    }
  }

  if (node->dependencies == nullptr) {
    node->capacity = 32;
    node->dependencies = realloc(node->dependencies, node->capacity * sizeof(DepNode *));
  }

  if (node->length >= node->capacity) {
    node->capacity *= 1.5;
    node->dependencies = realloc(node->dependencies, node->capacity * sizeof(DepNode *));
  }

  node->dependencies[node->length++] = dep;
}

void graph_builder_function_declaration(AST *node, DepNodeRegistry *registry, DepGraph *graph);
void graph_builder_type_declaration(AST *node, DepNodeRegistry *registry, DepGraph *graph);
void graph_builder_variable_declaration(AST *node, DepNodeRegistry *registry, DepNode *parent);
void graph_builder_function_call(AST *node, DepNodeRegistry *registry, DepNode *parent);
void graph_builder_binary_expression(AST *node, DepNodeRegistry *registry, DepNode *parent);
void graph_builder_return_statement(AST *node, DepNodeRegistry *registry, DepNode *parent);
void graph_builder_block(AST *node, DepNodeRegistry *registry, DepNode *parent);

static inline void populate_dep_graph(DepNodeRegistry *registry, DepGraph *graph, AST *root_node) {
  for (int i = 0; i < root_node->statements.length; ++i) {
    AST *statement = root_node->statements.data[i];
    switch (statement->kind) {
      case AST_NODE_FUNCTION_DECLARATION:
        graph_builder_function_declaration(statement, registry, graph);
        break;
      case AST_NODE_TYPE_DECLARATION:
        graph_builder_type_declaration(statement, registry, graph);
        break;
      default:
        break;
    }
  }
}

extern int node_printer_indentation;
static inline void print_node(DepNode *node) {
  for (int i = 0; i < node_printer_indentation; ++i) {
    printf("  ");
  }
  printf("node: %p, ast_node: %p, num_deps: %zu, error: %s\n", node, node->ast_node, node->length, node->error);
  node_printer_indentation++;
  for (int i = 0; i < node->length; ++i) {
    print_node(node->dependencies[i]);
  }
  node_printer_indentation--;
}

static inline void print_graph(DepGraph *graph) {
  for (int i = 0; i < graph->length; ++i) {
    DepNode *node = graph->nodes[i];
    print_node(node);
  }
}

#endif