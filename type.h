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
  LLVMTypeRef llvm_type;

  union {
    struct {
      Vector members;
    } $struct;
    struct {
      bool is_varargs;
      size_t $return;
      Vector parameters;
    } $function;
  };

} Type;

static Type *create_type(AST *declaring_node, String name, Type_Kind kind) {
  vector_push(&type_table, &(Type){
                               .name = name,
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

// parameter_types is Vector<size_t>
static Type *create_or_find_function_type(AST *declaring_node,
                                          size_t return_type,
                                          Vector parameter_types,
                                          bool is_varargs, bool *created) {
  *created = false;

  ForEachPtr(Type, type, type_table, {
    if (type->kind != FUNCTION)
      continue;

    typeof(type->$function) function = type->$function;

    if (is_varargs != function.is_varargs)

      continue;

    if (function.parameters.length != parameter_types.length)
      continue;

    bool match = true;
    for (int i = 0; i < function.parameters.length; ++i) {
      size_t a = ((size_t *)function.parameters.data)[i];
      size_t b = ((size_t *)parameter_types.data)[i];
      if (a != b) {
        match = false;
        break;
      }
    }

    if (!match)
      continue;

    if (type->$function.$return == return_type)
      continue;

    return type;
  });

  *created = true;
  // TODO: make a function type name?
  Type *type = create_type(declaring_node, (String){}, FUNCTION);
  type->$function.$return = return_type;
  type->$function.parameters = parameter_types;
  type->$function.is_varargs = is_varargs;
  return type;
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

static String type_to_string(Type *type) {
  switch (type->kind) {
    case VOID:
      return (String){.data = "void", .length = 4};
    case I32:
      return (String){.data = "i32", .length = 3};
    case F32:
      return (String){.data = "f32", .length = 3};
    case STRING:
      return (String){.data = "String", .length = 6};
    case STRUCT:
      if (type->name.data && type->name.length > 0)
        return type->name;
      else
        return (String){.data = "struct", .length = 6};
    case FUNCTION:
      // Print function type as: (param1, param2, ...) -> return_type
      static char buffer[256];
      size_t offset = 0;
      memcpy(buffer + offset, "fn(", 3);
      offset+=3;
      for (size_t i = 0; i < type->$function.parameters.length; ++i) {
        size_t param_type_id = ((size_t *)type->$function.parameters.data)[i];
        Type *param_type = get_type(param_type_id);
        String param_str = type_to_string(param_type);
        memcpy(buffer + offset, param_str.data, param_str.length);
        offset += param_str.length;
        if (i + 1 < type->$function.parameters.length) {
          buffer[offset++] = ',';
          buffer[offset++] = ' ';
        }
      }
      buffer[offset++] = ')';
      buffer[offset++] = ' ';
      Type *ret_type = get_type(type->$function.$return);
      String ret_str = type_to_string(ret_type);
      memcpy(buffer + offset, ret_str.data, ret_str.length);
      offset += ret_str.length;
      buffer[offset] = 0;
      return (String){.data = buffer, .length = offset};
    default:
      return (String){.data = "unknown", .length = 7};
  }
}

#endif