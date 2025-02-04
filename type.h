#ifndef TYPE_H
#define TYPE_H

#include "core.h"
#include <string.h>

typedef struct Type Type;

typedef struct Member {
  String name;
  Type *type;
} Type_Member;

typedef enum {
  VOID,
  I32,
  STRING,
  STRUCT,
} Type_Kind;

typedef struct Type {
  Type_Kind kind;
  String name;
  Type_Member members[12];
  size_t members_length;
} Type;

extern Type type_table[1024];
extern size_t type_table_length;

static Type *create_type(String name) {
  type_table[type_table_length] = (Type) {
    .name = name,
  };
  Type *type = &type_table[type_table_length];
  type_table_length++;
  return type;
}

static Type *find_type(String name) {
  for (int i = 0; i < type_table_length; ++i) {
    if (Strings_compare(type_table[i].name, name)) {
      return &type_table[i];
    }
  }
  return nullptr;
}

static Type_Member *find_member(String name, Type *type) {
  for (int i = 0; i < type->members_length; ++i) {
    if (Strings_compare(type->members[i].name, name)) {
      return &type->members[i];
    }
  }
  return nullptr;
}

static void initialize_type_system() {
  type_table[0] = (Type) {
    .name = (String) { 
      .start = "void",
      .length = 4
    },
    .kind = VOID,
  };
  type_table[1] = (Type) {
    .name = (String) { 
      .start = "i32",
      .length = 3
    },
    .kind = I32,
  };
  type_table[2] = (Type) {
    .name = (String) { 
      .start = "String",
      .length = 4
    },
    .kind = STRING,
  };
  type_table_length = 3;
}

#endif