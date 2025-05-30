
#ifndef THIR_H
#define THIR_H

#include "core.h"
#include "lexer.h"

typedef enum : u8 {
  THIR_PROGRAM,
  THIR_BLOCK,
  THIR_BINARY_EXPRESSION,
  THIR_CALL,
  THIR_MEMBER_ACCESS,
  THIR_IDENTIFIER,
  THIR_NUMBER,
  THIR_STRING,
  THIR_RETURN,
  THIR_FUNCTION,
  THIR_TYPE_DECLARATION,
  THIR_VARIABLE_DECLARATION,
} THIRKind;

typedef struct {
  String name;
  size_t type;
} THIRMember;

typedef struct {
  String name;
  size_t type;
  bool is_vararg;
} THIRParameter;

typedef struct THIR THIR;

typedef struct {
  THIR **nodes;
  size_t length;
  size_t capacity;
} THIRList;

static inline void thir_list_push(THIRList *list, THIR *node) {
  if (list->length >= list->capacity) {
    list->capacity = list->capacity ? list->capacity * 4 : 1;
    list->nodes = realloc(list->nodes, list->capacity * sizeof(THIR *));
  }
  list->nodes[list->length++] = node;
}

typedef struct THIR {
  size_t type;
  THIRKind kind;
  Source_Location location;

  union {
    // literals.
    String number;
    String string;

    struct {
      String name;
      THIR *resolved;
    } identifier;

    THIRList statements;      // block/program
    THIR *return_expression;  // expression for a return statement.

    struct {
      THIRList arguments;
      THIR *function;
    } call;

    struct {
      THIR *left;
      THIR *right;
      Token_Type operator;
    } binary;

    struct {
      THIR *base;
      String member;
    } member_access;

    struct {
      String name;
      bool is_extern, is_entry;
      THIR *block;
      Vector parameters;
    } function;

    struct {
      String name;
      THIR *value;
    } variable;

    struct {
      String name;
      Vector members;
    } type_declaration;
  };
} THIR;

typedef struct {
  String name;
  THIR *thir;
} THIRSymbol;

static inline THIRSymbol *find_thir_symbol(Vector *symbols, String name) {
  ForEachPtr(THIRSymbol, symbol, (*symbols), {
    if (Strings_compare(symbol->name, name)) {
      return symbol;
    }
  });
  return nullptr;
}

extern Arena thir_arena;

inline static void print_indent(int indent) {
  for (int i = 0; i < indent; ++i) printf("  ");
}
inline static void print_string(String str) { printf("%.*s", (int)str.length, str.data); }
#define THIRTypeNameCase(type) \
  case type:                   \
    return #type;

const char *thir_kind_to_string(THIRKind type);
void pretty_print_thir(THIR *thir, int indent);

#define THIR_ALLOC(tag, $location)                       \
  ({                                                     \
    THIR *thir = arena_alloc(&thir_arena, sizeof(THIR)); \
    memset(thir, 0, sizeof(THIR));                       \
    thir->kind = tag;                                    \
    thir->location = $location;                          \
    thir;                                                \
  })

#endif
