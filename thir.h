
#ifndef THIR_H
#define THIR_H

#include "core.h"
#include "lexer.h"
#include "type.h"

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
    list->nodes = (THIR **)realloc(list->nodes, list->capacity * sizeof(THIR *));
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

inline static void pretty_print_thir(THIR *thir, int indent);

inline static void print_indent(int indent) {
  for (int i = 0; i < indent; ++i) printf("  ");
}

inline static void print_string(String str) { printf("%.*s", (int)str.length, str.data); }

#define THIRTypeNameCase(type) \
  case type:                   \
    return #type;
static const char *thir_kind_to_string(THIRKind type) {
  switch (type) {
    THIRTypeNameCase(THIR_PROGRAM) THIRTypeNameCase(THIR_BLOCK) THIRTypeNameCase(THIR_BINARY_EXPRESSION)
        THIRTypeNameCase(THIR_CALL) THIRTypeNameCase(THIR_MEMBER_ACCESS) THIRTypeNameCase(THIR_IDENTIFIER)
            THIRTypeNameCase(THIR_NUMBER) THIRTypeNameCase(THIR_STRING) THIRTypeNameCase(THIR_RETURN)
                THIRTypeNameCase(THIR_FUNCTION) THIRTypeNameCase(THIR_TYPE_DECLARATION)
                    THIRTypeNameCase(THIR_VARIABLE_DECLARATION) default : return "INVALID_THIR_KIND";
  }
}

inline static void pretty_print_thir(THIR *thir, int indent) {
    if (!thir) {
        print_indent(indent);
        printf("<null>\n");
        return;
    }

    print_indent(indent);
    printf("<kind=(%d :: %s), type=(%s)>\n",
           thir->kind,
           thir_kind_to_string(thir->kind),
           type_to_string(get_type(thir->type)).data);

    switch (thir->kind) {
        case THIR_PROGRAM:
        case THIR_BLOCK:
            for (size_t i = 0; i < thir->statements.length; ++i) {
                pretty_print_thir(thir->statements.nodes[i], indent + 1);
            }
            break;

        case THIR_BINARY_EXPRESSION:
            pretty_print_thir(thir->binary.left, indent + 1);
            print_indent(indent + 1);
            printf("<operator :: '%s'>\n", Token_Type_Name(thir->binary.operator));
            pretty_print_thir(thir->binary.right, indent + 1);
            break;

        case THIR_CALL:
            print_indent(indent + 1);
            printf("<function :: %s>\n", thir->call.function->function.name.data);
            print_indent(indent + 1);
            printf("<args>\n");
            for (size_t i = 0; i < thir->call.arguments.length; ++i) {
              pretty_print_thir(thir->call.arguments.nodes[i], indent + 2);
            }
            break;

        case THIR_MEMBER_ACCESS:
            print_indent(indent + 1);
            printf("<base>\n");
            pretty_print_thir(thir->member_access.base, indent + 2);
            print_indent(indent + 1);
            printf("<member> ");
            print_string(thir->member_access.member);
            printf("\n");
            break;

        case THIR_IDENTIFIER:
            print_indent(indent + 1);
            printf("Identifier: ");
            print_string(thir->identifier.name);
            printf("\n");
            break;

        case THIR_NUMBER:
            print_indent(indent + 1);
            printf("Number: ");
            print_string(thir->number);
            printf("\n");
            break;

        case THIR_STRING:
            print_indent(indent + 1);
            printf("String: ");
            print_string(thir->string);
            printf("\n");
            break;

        case THIR_RETURN:
            print_indent(indent + 1);
            printf("Return\n");
            pretty_print_thir(thir->return_expression, indent + 2);
            break;

        case THIR_FUNCTION:
            print_indent(indent + 1);
            printf("Function: ");
            print_string(thir->function.name);
            printf("%s%s\n",
                   thir->function.is_extern ? " [extern]" : "",
                   thir->function.is_entry ? " [entry]" : "");
            print_indent(indent + 1);
            printf("<params>\n");
            for (size_t i = 0; i < thir->function.parameters.length; ++i) {
                THIRParameter *param = V_PTR_AT(THIRParameter, thir->function.parameters, i);
                print_indent(indent + 2);
                print_string(param->name);
                printf(" : %zu%s\n", param->type, param->is_vararg ? " (vararg)" : "");
            }
            print_indent(indent + 1);
            printf("<block>\n");
            pretty_print_thir(thir->function.block, indent + 2);
            break;

        case THIR_TYPE_DECLARATION:
            print_indent(indent + 1);
            printf("Type: ");
            print_string(thir->type_declaration.name);
            printf("\n");
            for (size_t i = 0; i < thir->type_declaration.members.length; ++i) {
                THIRMember *member = V_PTR_AT(THIRMember, thir->type_declaration.members, i);
                print_indent(indent + 2);
                print_string(member->name);
                printf(" : %zu\n", member->type);
            }
            break;

        case THIR_VARIABLE_DECLARATION:
            print_indent(indent + 1);
            printf("Variable: ");
            print_string(thir->variable.name);
            printf("\n");
            if (thir->variable.value) {
                print_indent(indent + 2);
                printf("Value:\n");
                pretty_print_thir(thir->variable.value, indent + 3);
            }
            break;

        default:
            print_indent(indent + 1);
            printf("<unknown THIR kind>\n");
            break;
    }
}

#define THIR_ALLOC(tag, $location)                       \
  ({                                                     \
    THIR *thir = arena_alloc(&thir_arena, sizeof(THIR)); \
    memset(thir, 0, sizeof(THIR));                       \
    thir->kind = tag;                                    \
    thir->location = $location;                          \
    thir;                                                \
  })

#endif
