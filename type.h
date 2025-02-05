#ifndef TYPE_H
#define TYPE_H

#include "core.h"
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
  AST *declaring_node;
  Type_Kind kind;
  String name;
  Type_Member members[12];
  size_t members_length;
  size_t size;
  size_t alignment;
} Type;

extern Type type_table[1024];
extern size_t type_table_length;

static size_t get_alignment(Type_Kind kind) {
  switch (kind) {
    case VOID: return 1;
    case I32: return 4;
    case STRING: return sizeof(void*); // Assuming pointers for strings
    case STRUCT: return 1; // Default, will be updated based on members
    default: return 1;
  }
}

static Type *create_type(AST *declaring_node, String name, Type_Kind kind, size_t size) {
  type_table[type_table_length] = (Type){
      .name = name,
      .declaring_node = declaring_node,
      .alignment = get_alignment(kind),
      .kind = kind,
      .size = size,
  };
  Type *type = &type_table[type_table_length];
  type_table_length++;
  return type;
}



static int64_t calculate_sizeof_type(Type *type) {
  if (type->members_length == 0) {
    return type->size;
  }
  int64_t size = 0;
  int64_t max_alignment = 1;
  for (int i = 0; i < type->members_length; ++i) {
    Type *member_type = type->members[i].type;
    int64_t member_size = calculate_sizeof_type(member_type);
    int64_t alignment = member_type->alignment;

    // Update max alignment
    if (alignment > max_alignment) {
      max_alignment = alignment;
    }

    // Add padding to align the member
    size = (size + alignment - 1) & ~(alignment - 1);

    // Add the member size
    size += member_size;
  }

  // Add padding to align the total size to the max alignment
  size = (size + max_alignment - 1) & ~(max_alignment - 1);

  type->size = size;
  return size;
}

static int64_t calculate_member_offset(Type *type, String member) {
  int64_t offset = 0;
  for (int i = 0; i < type->members_length; ++i) {
    if (Strings_compare(type->members[i].name, member)) {
      return offset;
    }
    int64_t alignment = type->members[i].type->alignment;
    offset = (offset + alignment - 1) & ~(alignment - 1);
    auto size = calculate_sizeof_type(type->members[i].type);
    offset += size;
  }
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
  for (int i = 0; i < type->members_length; ++i) {
    if (Strings_compare(type->members[i].name, name)) {
      return &type->members[i];
    }
  }
  return nullptr;
}

static void initialize_type_system() {
  create_type(nullptr, (String){.data = "void", .length = 4}, VOID, 0);
  create_type(nullptr, (String){.data = "i32", .length = 3}, I32, 4);
  create_type(nullptr, (String){.data = "String", .length = 6}, STRING, 8);
}

#endif