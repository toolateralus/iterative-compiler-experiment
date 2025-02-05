#include "emit.h"
#include "string_builder.h"

void emit_identifier(String_Builder *builder, AST *node) {
  string_builder_appendf(builder, "%.*s", node->identifier.length, node->identifier.data);
}

void emit_number(String_Builder *builder, AST *node) {
  string_builder_appendf(builder, "%.*s", node->number.length, node->number.data);
}

void emit_string(String_Builder *builder, AST *node) {
  string_builder_appendf(builder, "\"%.*s\"", node->string.length, node->string.data);
}

void emit_function_declaration(String_Builder *builder, AST *node) {
  if (node->emitted) return;
  node->emitted = true;
  auto decl = node->function_declaration;

  if (decl.is_extern) {
    string_builder_appendf(builder, "extern ");
  }

  if (decl.is_entry) {
    string_builder_appendf(builder, "int %.*s(", decl.name.length, decl.name.data);
  } else {
    string_builder_appendf(builder, "void %.*s(", decl.name.length, decl.name.data);
  }
    
  for (int i = 0; i < decl.parameters_length; ++i) {
    const auto param = decl.parameters[i];
    const char separator = (i == (decl.parameters_length - 1) ? ' ' : ',');
    if (param.is_varargs) {
      string_builder_appendf(builder, " ...%c", separator);
      continue;
    }
    
    if (param.name.length != 0) {
      string_builder_appendf(builder, "%.*s %.*s%c", param.type.length, param.type.data, param.name.length, param.name.data, separator);
    } else {
      string_builder_appendf(builder, "%.*s%c", param.type.length, param.type.data, separator);
    }
  }

  if (decl.is_extern) {
    string_builder_appendf(builder, ");\n");
    return;
  }

  string_builder_appendf(builder, ") ");
  emit_block(builder, decl.block);
}

void emit_type_declaration(String_Builder *builder, AST *node) {
  if (node->emitted) return;
  auto decl = node->type_declaration;
  string_builder_appendf(builder, "typedef struct %.*s {\n", decl.name.length, decl.name.data);
  for (int i = 0; i < decl.members_length; ++i) {
    auto member = decl.members[i];
    string_builder_appendf(builder, "\t%.*s %.*s;\n", member.type.length, member.type.data, member.name.length, member.name.data);
  }
  string_builder_appendf(builder, "} %.*s;\n", decl.name.length, decl.name.data);
  node->emitted = true;
}

void emit_variable_declaration(String_Builder *builder, AST *node) {
  string_builder_appendf(builder, "%.*s %.*s", node->variable_declaration.type.length, node->variable_declaration.type.data, node->variable_declaration.name.length, node->variable_declaration.name.data);
  if (node->variable_declaration.default_value) {
    string_builder_appendf(builder, " = ");
    emit(builder, node->variable_declaration.default_value);
  }
  string_builder_appendf(builder, ";\n");
}

void emit_assignment(String_Builder *builder, AST *node) {
  string_builder_appendf(builder, "%.*s = ", node->assignment.name.length, node->assignment.name.data);
  emit(builder, node->assignment.right);
  string_builder_appendf(builder, ";\n");
}

void emit_dot_expression(String_Builder *builder, AST *node) {
  string_builder_appendf(builder, "%.*s.%.*s", node->dot_expression.left.length, node->dot_expression.left.data, node->dot_expression.right.length, node->dot_expression.right.data);
  if (node->dot_expression.assignment_value) {
    string_builder_appendf(builder, " = ");
    emit(builder, node->dot_expression.assignment_value);
    string_builder_appendf(builder, ";\n");
  }
}

void emit_function_call(String_Builder *builder, AST *node) {
  if (node->emitted) return;
  string_builder_appendf(builder, "%.*s(", node->function_call.name.length, node->function_call.name.data);
  for (int i = 0; i < node->function_call.arguments_length; ++i) {
    emit(builder, node->function_call.arguments[i]);
    if (i != node->function_call.arguments_length - 1) {
      string_builder_appendf(builder, ", ");
    }
  }
  string_builder_appendf(builder, ");\n");
  node->emitted = true;
}

void emit_block(String_Builder *builder, AST *node) {
  string_builder_appendf(builder, "{\n");
  for (int i = 0; i < node->statements.length; ++i) {
    auto statement = node->statements.data[i];
    string_builder_appendf(builder, "\t");  
    emit(builder, statement);
  }
  string_builder_appendf(builder, "}\n");
}