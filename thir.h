
#ifndef THIR_H
#define THIR_H

#include "core.h"
#include "lexer.h"
#include "parser.h"
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
  THIR_FUNCTION_DECLARATION,
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
} THIRParameter;

typedef struct THIR THIR;
typedef struct THIR {
  size_t type;
  THIRKind kind;
  Source_Location source_location;

  union {
    // literals.
    String number;
    String string;

    struct {
      String name;
      THIR *resolved;
    } identifier;

    Vector statements; // block/program
    THIR *return_expression; // expression for a return statement.

    struct {
      Vector arguments;
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
      bool is_extern, 
            is_entry;
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
};

#endif
