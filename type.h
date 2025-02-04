#ifndef TYPE_H
#define TYPE_H

#include "core.h"
#include <string.h>

typedef struct Type Type;

typedef struct Member {
  String name;
  Type *type;
} Member;

typedef enum {
  I32,
  STRING,
  STRUCT,
} TypeKind;

typedef struct Type {
  TypeKind kind;
  String name;
  Member members[12];
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
    auto longest = name.length > type_table[i].name.length
                       ? name.length
                       : type_table[i].name.length;

    if (strncmp(type_table[i].name.start, name.start, longest) == 0) {
      return &type_table[i];
    }
  }
  return nullptr;
}

static Member *find_member(String name, Type *type) {
  for (int i = 0; i < type->members_length; ++i) {
    auto longest = name.length > type->members[i].name.length
                       ? name.length
                       : type->members[i].name.length;

    if (strncmp(type->members[i].name.start, name.start, longest) == 0) {
      return &type->members[i];
    }
  }
  return nullptr;
}

#endif