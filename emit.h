#ifndef EMIT_H
#define EMIT_H

#include "parser.h"
#include <stdarg.h>
#include "string_builder.h"

void emit_identifier(String_Builder *builder, AST *node);
void emit_number(String_Builder *builder, AST *node);
void emit_string(String_Builder *builder, AST *node);
void emit_function_declaration(String_Builder *builder, AST *node);
void emit_type_declaration(String_Builder *builder, AST *node);
void emit_variable_declaration(String_Builder *builder, AST *node);
void emit_assignment(String_Builder *builder, AST *node);
void emit_dot_expression(String_Builder *builder, AST *node);
void emit_function_call(String_Builder *builder, AST *node);
void emit_block(String_Builder *builder, AST *node);

static inline void emit(String_Builder *builder, AST *node) {
  switch (node->kind) {
  case AST_NODE_IDENTIFIER:
    return emit_identifier(builder, node);
  case AST_NODE_NUMBER:
    return emit_number(builder, node);
  case AST_NODE_STRING:
    return emit_string(builder, node);
  case AST_NODE_FUNCTION_DECLARATION:
    return emit_function_declaration(builder, node);
  case AST_NODE_TYPE_DECLARATION:
    return emit_type_declaration(builder, node);
  case AST_NODE_VARIABLE_DECLARATION:
    return emit_variable_declaration(builder, node);
  case AST_NODE_ASSIGNMENT:
    return emit_assignment(builder, node);
  case AST_NODE_DOT_EXPRESSION:
    return emit_dot_expression(builder, node);
  case AST_NODE_FUNCTION_CALL:
    return emit_function_call(builder, node);
  case AST_NODE_PROGRAM:
  case AST_NODE_BLOCK:
    return emit_block(builder, node);
  }
}

#endif