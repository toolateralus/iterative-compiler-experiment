#ifndef TYPE_H
#define TYPE_H

#include "core.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdint.h>

typedef struct Type Type;

typedef struct Member {
  String name;
  size_t type;
} Type_Member;

typedef enum {
  VOID,
  I32,
  F32,
  STRING,

  STRUCT,
  FUNCTION,
} Type_Kind;


extern Vector type_table;

typedef struct AST AST;

typedef struct Type {
  String name;
  Type_Kind kind;
  size_t id;
  AST *declaring_node;
  LLVMTypeRef llvm_type;

  union {
    struct {
      Vector members;
    } $struct;
    struct {
      size_t $return;
      Vector parameters;
    } $function;
  };

} Type;

static Type *create_type(AST *declaring_node, String name, Type_Kind kind) {
  vector_push(&type_table, &(Type){
                               .name = name,
                               .declaring_node = declaring_node,
                               .kind = kind,
                               .id = type_table.length,
                           });

  Type *type = V_PTR_BACK(Type, type_table);

  if (kind == STRUCT)
    vector_init(&type->$struct.members, sizeof(Type_Member));

  return type;
}

static int8_t get_member_index(Type *type, String member_name) {
  if (type->kind != STRUCT)
    return -1;
  ForEach(Type_Member, member, type->$struct.members, {
    if (Strings_compare(member.name, member_name)) {
      return i;
    }
  });
  return -1;
}

static Type *find_type(String name) {
  ForEachPtr(Type, type, type_table, {
    if (Strings_compare(type->name, name)) {
      return type;
    }
  });
  return nullptr;
}

static Type_Member *find_member(Type *type, String name) {
  ForEachPtr(Type_Member, member, type->$struct.members, {
    if (Strings_compare(member->name, name)) {
      return member;
    }
  }) return nullptr;
}

static Type *get_type(size_t id) {
  if (id > type_table.length) 
    return nullptr;
  return V_PTR_AT(Type, type_table, id);
}

static void initialize_type_system() {
  vector_init(&type_table, sizeof(Type));
  create_type(nullptr, (String){.data = "void", .length = 4}, VOID);
  create_type(nullptr, (String){.data = "i32", .length = 3}, I32);
  create_type(nullptr, (String){.data = "f32", .length = 3}, F32);
  create_type(nullptr, (String){.data = "String", .length = 6}, STRING);
}

#endif