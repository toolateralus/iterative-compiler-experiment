#ifndef PARSER_H
#define PARSER_H

#include "core.h"
#include <assert.h>
#include "lexer.h"
#include "type.h"
#include <llvm-c/Types.h>
#include <stdarg.h>

typedef enum: u8 {
  AST_NODE_PROGRAM,
  AST_NODE_IDENTIFIER,
  AST_NODE_NUMBER,
  AST_NODE_STRING,
  AST_NODE_FUNCTION_DECLARATION,
  AST_NODE_TYPE_DECLARATION,
  AST_NODE_VARIABLE_DECLARATION,
  AST_NODE_DOT_EXPRESSION,
  AST_NODE_FUNCTION_CALL,
  AST_NODE_BLOCK,
  AST_NODE_BINARY_EXPRESSION,
  AST_NODE_RETURN,
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
  size_t type;
  struct Symbol *next;
  LLVMValueRef llvm_value;
  LLVMTypeRef llvm_function_type;
} Symbol;

typedef struct AST {
  bool typing_complete: 1;
  AST_Node_Kind kind;
  size_t type;
  Symbol symbol_table;
  Source_Location location;
  struct AST *parent;

  union {
    struct {
      String name;
      String return_type;

      // todo: add function flags?
      bool is_extern : 1, 
           is_entry : 1;

      Vector parameters;
      struct AST *block;
    } function;

    struct {
      String type;
      String name;
      struct AST *value;
    } variable;

    struct {
      String name;
      Vector arguments;
    } call;

    struct {
      String name;
      Vector members;
    } declaration;

    struct {
      struct AST *left;
      String member_name;
    } dot;

    struct {
      AST *left;
      AST *right;
      Token_Type operator;
    } binary;

    struct AST *$return;
    AST_List statements;
    String string;
    String identifier;
    String number;
  };
} AST;


[[noreturn]]
static void parse_panic(Source_Location location, const char * message) {
  fprintf(stderr, "at: %s:%d:%d\nerror: %s\n", location.file, location.line, location.line, message);
  exit(1);
}

[[noreturn]]
static void parse_panicf(Source_Location location, const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "at: %s:%d:%d\nerror: ", location.file, location.line, location.line);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

static int64_t get_parameter_index(AST *node, String name) {
  assert(node->kind == AST_NODE_FUNCTION_DECLARATION && "get_parameter_index called on a non-function node");
  auto func = node->function;
  ForEach(AST_Parameter, param, func.parameters, {
    if (Strings_compare(name, param.name)) {
      return i;
    }
  });
  return -1;
}

Symbol *find_symbol(AST *scope, String name);

void insert_symbol(AST *scope, String name, AST *node, Type *type);

typedef struct AST_Arena {
  AST nodes[1024];
  size_t nodes_length;
  struct AST_Arena *next;
} AST_Arena;

static inline AST *ast_arena_alloc(Lexer_State* state, AST_Arena *arena, AST_Node_Kind kind, AST *parent) {
  if (arena->nodes_length < 1024) {
    AST *node = &arena->nodes[arena->nodes_length++];
    node->kind = kind;
    node->type = 0;
    node->location = state->location;
    node->parent = parent;
    return node;
  } else {
    if (arena->next == NULL) {
      arena->next = (AST_Arena *)malloc(sizeof(AST_Arena));
      arena->next->nodes_length = 0;
      arena->next->next = NULL;
    }
    return ast_arena_alloc(state, arena->next, kind, parent);
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
AST *parse_binary_expression(AST_Arena *arena, Lexer_State *state, AST *parent);
AST *parse_postfix_expression(AST_Arena *arena, Lexer_State *state,
                              AST *parent);

#define ast_list_push(list, node) do { \
  if ((list)->length >= (list)->capacity) { \
    (list)->capacity = (list)->capacity ? (list)->capacity * 4 : 1; \
    (list)->data = (AST **)realloc((list)->data, (list)->capacity * sizeof(AST *)); \
  } \
  (list)->data[(list)->length++] = (node); \
} while (0)

#endif