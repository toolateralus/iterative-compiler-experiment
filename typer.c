#include "typer.h"
#include "core.h"
#include "parser.h"
#include "type.h"
#include <assert.h>

Typer_Progress typer_identifier(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  auto symbol = find_symbol(node->parent, node->identifier);
  if (!symbol) {
    return UNRESOLVED;
  } else {
    node->typing_complete = true;
    node->type = symbol->type;
    return COMPLETE;
  }
}

Typer_Progress typer_number(AST *node) {
  if (node->typing_complete)
    return COMPLETE;
  node->type = I32;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_string(AST *node) {
  if (node->typing_complete)
    return COMPLETE;
  node->type = STRING;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_function_declaration(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  Vector parameter_types;
  vector_init(&parameter_types, sizeof(size_t));
  bool is_varargs = false;
  for (size_t i = 0; i < node->function_declaration.parameters.length; ++i) {
    AST_Parameter *parameter =
        V_PTR_AT(AST_Parameter, node->function_declaration.parameters, i);
    if (parameter->is_varargs) {
      is_varargs = true;
      continue;
    }

    Type *param_type = find_type(parameter->type);
    vector_push(&parameter_types, &param_type->id);

    if (!param_type)
      return UNRESOLVED;
    if (node->function_declaration.is_extern)
      continue;
    insert_symbol(node, parameter->name, node, param_type);
  }

  auto return_ty = find_type(node->function_declaration.return_type);

  if (!return_ty) {
    return UNRESOLVED;
  }

  if (!node->function_declaration.is_extern) {
    // Resolve the function body
    Typer_Progress body_progress =
        typer_resolve(node->function_declaration.block);
    if (body_progress != COMPLETE)
      return UNRESOLVED;
  }



  bool created;
  Type *type = create_or_find_function_type(node, return_ty->id, parameter_types, is_varargs, &created);

  // If we didn't create this type, we need to clean up these params.
  // If we did, these now are owned by the type, we just copy the vector.
  if (!created) {
    vector_free(&parameter_types);
  }

  insert_symbol(node->parent, node->function_declaration.name, node, type);
  node->type = type->id;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_type_declaration(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  Type *type = create_type(node, node->type_declaration.name, STRUCT);

  ForEach(AST_Type_Member, member, node->type_declaration.members, {
    Type *member_type = find_type(member.type);
    if (!member_type)
      return UNRESOLVED;
    insert_symbol(node, member.name, node, member_type);
    Type_Member type_member;
    type_member.name = member.name;
    type_member.type = member_type->id;
    vector_push(&type->$struct.members, &type_member);
  });

  node->type = VOID;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_variable_declaration(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  typeof(node->variable_declaration) *decl = &node->variable_declaration;

  Typer_Progress value_progress = COMPLETE;
  if (decl->default_value)
    value_progress = typer_resolve(decl->default_value);

  if (value_progress != COMPLETE)
    return UNRESOLVED;

  Type *type = find_type(decl->type);
  if (!type)
    return UNRESOLVED;

  if (decl->default_value)
    assert(decl->default_value->type == type->id && "Invalid type in declaration");

  insert_symbol(node->parent, decl->name, node, type);

  node->type = VOID;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_assignment(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  Typer_Progress value_progress = typer_resolve(node->assignment.right);
  if (value_progress != COMPLETE)
    return UNRESOLVED;

  Symbol *symbol = find_symbol(node->parent, node->assignment.name);
  if (!symbol)
    return UNRESOLVED;

  if (symbol->type != node->assignment.right->type) {
    fprintf(stderr, "invalid types in assignment\n");
    exit(1);
  }
  node->type = symbol->type;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_dot_expression(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  Symbol *left_symbol = find_symbol(node->parent, node->dot_expression.left);

  if (!left_symbol)
    return UNRESOLVED;

  Type *left_type = get_type(left_symbol->type);
  if (!left_type)
    return UNRESOLVED;

  Type_Member *member = find_member(left_type, node->dot_expression.right);

  if (!member) {
    fprintf(stderr, "cannot find member %s in type %s\n",
            node->dot_expression.right.data, get_type(left_symbol->type)->name.data);
    exit(1);
  }

  if (node->dot_expression.assignment_value) {
    Typer_Progress progress =
        typer_resolve(node->dot_expression.assignment_value);
    if (progress != COMPLETE) {
      return UNRESOLVED;
    }
  }

  node->type = member->type;
  node->typing_complete = true;

  return COMPLETE;
}

Typer_Progress typer_function_call(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  Symbol *symbol = find_symbol(node->parent, node->function_call.name);

  if (!symbol)
    return UNRESOLVED;

  Type *type = get_type(symbol->type);

  assert(type->kind == FUNCTION && "Attempted to call a non-function type");

  typeof(type->$function) function_type = type->$function;

  if ((node->function_call.arguments.length != function_type.parameters.length) && !function_type.is_varargs) 
    parse_panicf(node->location, "expected #%d arguments, got #%d", function_type.parameters.length, node->function_call.arguments.length);

  // TODO: type check arguments,
  // TODO: the challenge is handling varargs.
  for (size_t i = 0; i < node->function_call.arguments.length; ++i) {
    Typer_Progress arg_progress =
        typer_resolve(V_AT(AST *, node->function_call.arguments, i));
    if (arg_progress != COMPLETE)
      return UNRESOLVED;
  }

  node->type = type->$function.$return;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_binary_expression(AST *node) {
  if (node->typing_complete)
    return COMPLETE;

  Typer_Progress left_progress = typer_resolve(node->binary_expression.left);
  if (left_progress != COMPLETE)
    return UNRESOLVED;

  Typer_Progress right_progress = typer_resolve(node->binary_expression.right);
  if (right_progress != COMPLETE)
    return UNRESOLVED;

  if (node->binary_expression.left->type != node->binary_expression.right->type) {
    fprintf(stderr, "invalid types in binary expression\n");
    exit(1);
  }

  node->type = node->binary_expression.left->type;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_block(AST *node) {
  if (node->typing_complete)
    return COMPLETE;
  // Resolve each statement in the block
  for (size_t i = 0; i < node->statements.length; ++i) {
    Typer_Progress stmt_progress = typer_resolve(node->statements.data[i]);
    if (stmt_progress != COMPLETE)
      return UNRESOLVED;
  }
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_return_statement(AST *node) {
  if (node->$return.value) {
    return typer_resolve(node->$return.value);
  }
  return COMPLETE;
}
