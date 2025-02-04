#include "typer.h"
#include "parser.h"
#include "type.h"

Typer_Progress typer_identifier(AST *node) {
  if (node->typing_complete) return COMPLETE;
  auto symbol = find_symbol(node->parent, node->identifier);
  if (!symbol) {
    return UNRESOLVED;
  }
  else {
    node->typing_complete = true;
    node->type = symbol->type;
    return COMPLETE;
  }
}

Typer_Progress typer_number(AST *node) {
  if (node->typing_complete) return COMPLETE;
  const static String string = {
      .start = "i32",
      .length = 3,
  };
  node->type = find_type(string);
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_string(AST *node) {
  if (node->typing_complete) return COMPLETE;
  const static String string = {
      .start = "String",
      .length = 6,
  };
  node->type = find_type(string);
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_function_declaration(AST *node) {
  if (node->typing_complete) return COMPLETE;

  // Resolve parameter types
  for (size_t i = 0; i < node->function_declaration.parameters_length; ++i) {
    AST_Parameter *parameter = &node->function_declaration.parameters[i];
    if (parameter->is_varargs) {
      continue;
    }

    Type *param_type = find_type(parameter->type);
    
    if (!param_type)
      return UNRESOLVED;

    if (node->function_declaration.is_extern) continue;

    insert_symbol(node, parameter->name, node,
                  param_type);
  }

  if (!node->function_declaration.is_extern) {
    // Resolve the function body
    Typer_Progress body_progress = typer_resolve(node->function_declaration.body);
    if (body_progress != COMPLETE)
      return UNRESOLVED;
  }

  insert_symbol(node->parent, node->function_declaration.name, node, nullptr);
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_type_declaration(AST *node) {
  if (node->typing_complete) return COMPLETE;
  // Resolve member types
  for (size_t i = 0; i < node->type_declaration.members_length; ++i) {
    Type *member_type = find_type(node->type_declaration.members[i].type);
    if (!member_type)
      return UNRESOLVED;
    insert_symbol(node, node->type_declaration.members[i].name, node,
                  member_type);
  }
  Type *type = create_type(node->type_declaration.name);

  for (size_t i = 0; i < node->type_declaration.members_length; ++i) {
    Type *member_type = find_type(node->type_declaration.members[i].type);
    type->members[i].name = node->type_declaration.members[i].name;
    type->members[i].type = member_type;
    type->members_length++;
  }
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_variable_declaration(AST *node) {
  if (node->typing_complete) return COMPLETE;
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
    assert(decl->default_value->type == type && "Invalid type in declaration");

  insert_symbol(node->parent, decl->name, node, type);

  node->type = type;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_assignment(AST *node) {
  if (node->typing_complete) return COMPLETE;
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

  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_dot_expression(AST *node) {
  if (node->typing_complete) return COMPLETE;
  Symbol *left_symbol = find_symbol(node->parent, node->dot_expression.left);
  if (!left_symbol)
    return UNRESOLVED;

  Type *left_type = left_symbol->type;
  if (!left_type || !find_member(left_type, node->dot_expression.right))
    return UNRESOLVED;

  node->type = find_member(left_type, node->dot_expression.right)->type;
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_function_call(AST *node) {
  if (node->typing_complete) return COMPLETE;
  Symbol *symbol = find_symbol(node->parent, node->function_call.name);
  if (!symbol)
    return UNRESOLVED;

  for (size_t i = 0; i < node->function_call.arguments_length; ++i) {
    Typer_Progress arg_progress =
        typer_resolve(node->function_call.arguments[i]);
    if (arg_progress != COMPLETE)
      return UNRESOLVED;
  }
  node->typing_complete = true;
  return COMPLETE;
}

Typer_Progress typer_block(AST *node) {
  if (node->typing_complete) return COMPLETE;
  // Resolve each statement in the block
  for (size_t i = 0; i < node->statements.length; ++i) {
    Typer_Progress stmt_progress = typer_resolve(node->statements.data[i]);
    if (stmt_progress != COMPLETE)
      return UNRESOLVED;
  }
  node->typing_complete = true;
  return COMPLETE;
}