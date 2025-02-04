#ifndef PARSER_H
#define PARSER_H

#include "core.h"
#include "lexer.h"
#include "type.h"

typedef enum {
  AST_NODE_PROGRAM,
  AST_NODE_IDENTIFIER,
  AST_NODE_NUMBER,
  AST_NODE_STRING,

  AST_NODE_FUNCTION_DECLARATION,
  AST_NODE_TYPE_DECLARATION,
  AST_NODE_VARIABLE_DECLARATION,

  AST_NODE_ASSIGNMENT,
  AST_NODE_DOT_EXPRESSION,
  AST_NODE_FUNCTION_CALL,
  
  AST_NODE_BLOCK,
} AST_Node_Kind;

static const char *Node_Kind_String[] = {
  "AST_NODE_PROGRAM",
  "AST_NODE_IDENTIFIER",
  "AST_NODE_NUMBER",
  "AST_NODE_STRING",
  "AST_NODE_FUNCTION_DECLARATION",
  "AST_NODE_TYPE_DECLARATION",
  "AST_NODE_VARIABLE_DECLARATION",
  "AST_NODE_ASSIGNMENT",
  "AST_NODE_DOT_EXPRESSION",
  "AST_NODE_FUNCTION_CALL",
  "AST_NODE_BLOCK",
};

typedef struct {
  String type;
  String name;
  bool is_varargs;
} AST_Parameter;

typedef struct {
  String type;
  String name;
} AST_Type_Member;

typedef struct AST AST;

typedef struct AST_List {
  AST **data;
  size_t length;
  size_t capacity;
} AST_List;

typedef struct Symbol {
  String name;
  AST *node;
  Type *type;
  struct Symbol *next;
} Symbol;

typedef struct AST {
  bool typing_complete;
  AST_Node_Kind kind;
  Type *type;
  Symbol symbol_table;

  struct AST *parent;
  union {
    struct {
      String name;
      bool is_extern, is_entry;
      AST_Parameter parameters[12];
      size_t parameters_length;
      struct AST *body;
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
      AST_Type_Member members[12];
      size_t members_length;
    } type_declaration;

    struct {
      String left;
      String right;
      struct AST *assignment_value;
    } dot_expression;

    struct {
      String name;
      struct AST *right;
    } assignment;

    AST_List statements;

    String string;
    String identifier;
    String number;
  };
} AST;

Symbol *find_symbol(AST *scope, String name);

void insert_symbol(AST *scope, String name, AST *node, Type *type);

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
    case AST_NODE_PROGRAM:
    case AST_NODE_BLOCK: {
      memset(&node->statements, 0, sizeof(node->statements));
    } break;
    case AST_NODE_IDENTIFIER: {
      memset(&node->identifier, 0, sizeof(node->identifier));
    } break;
    case AST_NODE_NUMBER: {
      memset(&node->number, 0, sizeof(node->number));
    } break;
    case AST_NODE_STRING: {
      memset(&node->string, 0, sizeof(node->string));
    } break;
    case AST_NODE_FUNCTION_DECLARATION: {
      memset(&node->function_declaration, 0, sizeof(node->function_declaration));
    } break;
    case AST_NODE_TYPE_DECLARATION: {
      memset(&node->type_declaration, 0, sizeof(node->type_declaration));
    } break;
    case AST_NODE_VARIABLE_DECLARATION: {
      memset(&node->variable_declaration, 0, sizeof(node->variable_declaration));
    } break;
    case AST_NODE_ASSIGNMENT: {
      memset(&node->assignment, 0, sizeof(node->assignment));
    } break;
    case AST_NODE_DOT_EXPRESSION: {
      memset(&node->dot_expression, 0, sizeof(node->dot_expression));
    } break;
    case AST_NODE_FUNCTION_CALL: {
      memset(&node->function_call, 0, sizeof(node->function_call));
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

AST *parse_next_statement(AST_Arena *arena, Lexer_State *state, AST *parent);
AST *parse_block(AST_Arena *arena, Lexer_State *state, AST *parent);
AST *parse_expression(AST_Arena *arena, Lexer_State *state, AST *parent);
AST *parse_function_declaration(AST_Arena *arena, Lexer_State *state, AST *parent);
AST *parse_type_declaration(AST_Arena *arena, Lexer_State *state, AST *parent);


static void ast_list_push(AST_List *list, AST *node) {
  if (list->length >= list->capacity) {
    list->capacity = list->capacity ? list->capacity * 4 : 1;
    list->data = (AST **)realloc(list->data, list->capacity * sizeof(AST *));
  }
  list->data[list->length++] = node;
}

static AST *ast_list_pop(AST_List *list) {
  if (list->length == 0) {
    return NULL;
  }
  return list->data[--list->length];
}
#endif