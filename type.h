#ifndef TYPE_H
#define TYPE_H

#include "core.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdint.h>

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

typedef struct AST AST;
typedef struct Type {
  String name;
  AST *declaring_node;
  Type_Kind kind;
  Vector members;
  size_t size;
  LLVMTypeRef llvm_type;
} Type;

extern Type type_table[1024];
extern size_t type_table_length;

static size_t get_alignment(Type_Kind kind) {
  switch (kind) {
  case VOID:
    return 1;
  case I32:
    return 4;
  case STRING:
    return sizeof(void *); // Assuming pointers for strings
  case STRUCT:
    return 1; // Default, will be updated based on members
  default:
    return 1;
  }
}

static Type *create_type(AST *declaring_node, String name, Type_Kind kind,
                         size_t size) {
  type_table[type_table_length] = (Type){
      .name = name,
      .declaring_node = declaring_node,
      .kind = kind,
      .size = size,
  };
  Type *type = &type_table[type_table_length];
  vector_init(&type->members, sizeof(Type_Member));
  type_table_length++;
  return type;
}

static int8_t get_member_index(Type *type, String member_name) {
  ForEach(Type_Member, member, type->members, {
    if (Strings_compare(member.name, member_name)) {
      return i;
    }
  });
  return -1;
}

static Type *find_type(String name) {
  for (int i = 0; i < type_table_length; ++i) {
    if (Strings_compare(type_table[i].name, name)) {
      return &type_table[i];
    }
  }
  return nullptr;
}

static Type_Member *find_member(Type *type, String name) {
  {
    for (int i = 0; i < type->members.length; ++i) {
      Type_Member *member = ((Type_Member *)vector_get(&type->members, i));
      {
        if (Strings_compare(member->name, name)) {
          return member;
        }
      }
    }
  };
  return nullptr;
}

static void initialize_type_system() {
  create_type(nullptr, (String){.data = "void", .length = 4}, VOID, 0);
  create_type(nullptr, (String){.data = "i32", .length = 3}, I32, 4);
  create_type(nullptr, (String){.data = "String", .length = 6}, STRING, 8);
}

#endif