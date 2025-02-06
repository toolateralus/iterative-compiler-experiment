#ifndef TYPER_H
#define TYPER_H

#include "parser.h"

typedef enum {
  UNRESOLVED,
  COMPLETE,
} Typer_Progress;

Typer_Progress typer_identifier(AST *node);
Typer_Progress typer_number(AST *node);
Typer_Progress typer_string(AST *node);
Typer_Progress typer_function_declaration(AST *node);
Typer_Progress typer_type_declaration(AST *node);
Typer_Progress typer_variable_declaration(AST *node);
Typer_Progress typer_assignment(AST *node);
Typer_Progress typer_dot_expression(AST *node);
Typer_Progress typer_function_call(AST *node);
Typer_Progress typer_binary_expression(AST *node);
Typer_Progress typer_block(AST *node);

static inline Typer_Progress typer_resolve(AST *node) {
  switch (node->kind) {
  case AST_NODE_IDENTIFIER:
    return typer_identifier(node);
  case AST_NODE_NUMBER:
    return typer_number(node);
  case AST_NODE_STRING:
    return typer_string(node);
  case AST_NODE_FUNCTION_DECLARATION:
    return typer_function_declaration(node);
  case AST_NODE_TYPE_DECLARATION:
    return typer_type_declaration(node);
  case AST_NODE_VARIABLE_DECLARATION:
    return typer_variable_declaration(node);
  case AST_NODE_ASSIGNMENT:
    return typer_assignment(node);
  case AST_NODE_DOT_EXPRESSION:
    return typer_dot_expression(node);
  case AST_NODE_FUNCTION_CALL:
    return typer_function_call(node);
  case AST_NODE_BINARY_EXPRESSION: 
    return typer_binary_expression(node);
  case AST_NODE_PROGRAM:
  case AST_NODE_BLOCK:
    return typer_block(node);
  }
}

#endif