#include "graph.h"
#include "core.h"
#include "parser.h"

void graph_builder_variable_declaration(AST *node, DepNodeRegistry *registry, DepNode *parent) {
  Symbol *symbol = find_symbol(node->parent, node->variable.type);
  if (symbol && symbol->node) {
    add_dep_to_dep_node(parent, create_dep_node(symbol->node, registry));
  }
}
void graph_builder_function_call(AST *node, DepNodeRegistry *registry, DepNode *parent) {
  Symbol *symbol = find_symbol(node->parent, node->call.name);
  add_dep_to_dep_node(parent, create_dep_node(symbol->node, registry));
}
void graph_builder_binary_expression(AST *node, DepNodeRegistry *registry, DepNode *parent) {
  add_dep_to_dep_node(parent, create_dep_node(node->binary.left, registry));
  add_dep_to_dep_node(parent, create_dep_node(node->binary.right, registry));
}
void graph_builder_return_statement(AST *node, DepNodeRegistry *registry, DepNode *parent) {
  if (node->return_expression) {
    add_dep_to_dep_node(parent, create_dep_node(node->return_expression, registry));
  }
}
void graph_builder_block(AST *node, DepNodeRegistry *registry, DepNode *parent) {
  for (int i = 0; i < node->statements.length; ++i) {
    AST *statement = node->statements.data[i];
    switch (statement->kind) {
      case AST_NODE_VARIABLE_DECLARATION:
        return graph_builder_variable_declaration(statement, registry, parent);
      case AST_NODE_FUNCTION_CALL:
        return graph_builder_function_call(statement, registry, parent);
      case AST_NODE_BLOCK:
        return graph_builder_block(statement, registry, parent);
      case AST_NODE_BINARY_EXPRESSION:
        return graph_builder_binary_expression(statement, registry, parent);
      case AST_NODE_RETURN:
        return graph_builder_return_statement(statement, registry, parent);
        break;
      default:
        break;
    }
  }
}

void graph_builder_function_declaration(AST *node, DepNodeRegistry *registry, DepGraph *graph) {
  DepNode *dep_node = create_dep_node(node, registry);
  add_node_to_dep_graph(graph, dep_node);

  {
    for (int i = 0; i < node->function.parameters.length; ++i) {
      AST_Parameter *parameter = ((AST_Parameter *)vector_get(&node->function.parameters, i));
      {
        if (parameter->is_varargs) {
          continue;
        }
        Symbol *symbol = find_symbol(node->parent, parameter->type);

        // for shit like String etc, we won't get a node. and these don't need to be recoreded since they're built in.
        if (symbol && symbol->node) {
          add_dep_to_dep_node(dep_node, create_dep_node(symbol->node, registry));
        }
      }
    }
  };

  if (node->function.is_extern) {
    return;
  }

  graph_builder_block(node->function.block, registry, dep_node);
}

void graph_builder_type_declaration(AST *node, DepNodeRegistry *registry, DepGraph *graph) {
  DepNode *dep_node = create_dep_node(node, registry);

  ForEachPtr(AST_Type_Member, member, node->declaration.members, {
    Symbol *symbol = find_symbol(node->parent, member->type);
    if (symbol && symbol->node) {
      add_dep_to_dep_node(dep_node, create_dep_node(symbol->node, registry));
    }
  });

  add_node_to_dep_graph(graph, dep_node);
}