#ifndef PARSER_H
#define PARSER_H

#include "core.h"
#include "type.h"

typedef enum {
  AST_NODE_IDENTIFIER,
  AST_NODE_NUMBER,
  AST_NODE_STRING,

  AST_NODE_FUNCTION_DECLARATION,
  AST_NODE_TYPE_DECLARATION,
  AST_NODE_VARIABLE_DECLARATION,

  AST_NODE_ASSIGNMENT,
  AST_NODE_DOT_EXPRESSION,
  AST_NODE_FUNCTION_CALL,

} AST_Node_Kind;

typedef struct {
  String type;
  String name;
} Parameter;


typedef struct AST {
  AST_Node_Kind kind;
  Type *type;
  union {
    struct {
      bool is_extern, is_entry;
      Parameter parameters[12];
      size_t parameters_length;
    } function_declaration;

    struct {
      String type;
      String name;
      struct AST *default_value;
    } variable_declaration;

    struct {
      String name;
      struct AST *arguments[12];
      size_t arguments_length;
    } function_call;

    struct {
      String name;
      Member members[12];
    } type_declaration;

    struct {
      String left;
      String right;
    } dot_expression;

    struct {
      String name;
      struct AST *right;
    } assignment;

    String string;
    String identifier;
    String number;
  };
} AST;

typedef struct AST_Arena {
  AST nodes[1024];
  size_t nodes_length;
  struct AST_Arena *next;
} AST_Arena;

static AST *ast_arena_alloc(AST_Arena *arena, AST_Node_Kind kind) {
  if (arena->nodes_length < 1024) {
    AST *node = &arena->nodes[arena->nodes_length++];
    node->kind = kind;
    switch (kind) {
    case AST_NODE_IDENTIFIER: {
      node->identifier = (String){0};
    } break;
    case AST_NODE_NUMBER: {
      node->number = (String){0};
    } break;
    case AST_NODE_STRING: {
      node->string = (String){0};
    } break;
    case AST_NODE_FUNCTION_DECLARATION: {
      node->function_declaration = (typeof(node->function_declaration)){0};
    } break;
    case AST_NODE_TYPE_DECLARATION: {
      node->type_declaration = (typeof(node->type_declaration)){0};
    } break;
    case AST_NODE_VARIABLE_DECLARATION: {
      node->variable_declaration = (typeof(node->variable_declaration)){0};
    } break;
    case AST_NODE_ASSIGNMENT: {
      node->assignment = (typeof(node->assignment)){0};
    } break;
    case AST_NODE_DOT_EXPRESSION: {
      node->dot_expression = (typeof(node->dot_expression)){0};
    } break;
    case AST_NODE_FUNCTION_CALL: {
      node->function_call = (typeof(node->function_call)){0};
    } break;
    }
    node->type = nullptr;
    return node;
  } else {
    if (arena->next == NULL) {
      arena->next = (AST_Arena *)calloc(1, sizeof(AST_Arena));
      arena->next->nodes_length = 0;
      arena->next->next = NULL;
    }
    return ast_arena_alloc(arena->next, kind);
  }
}

static void ast_arena_free(AST_Arena *arena) {
  while (arena != NULL) {
    AST_Arena *next = arena->next;
    free(arena);
    arena = next;
  }
}

#endif