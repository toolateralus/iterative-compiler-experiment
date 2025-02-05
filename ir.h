#ifndef IR_H
#define IR_H
#include "core.h"
#include "parser.h"
#include "type.h"
#include <assert.h>
#include <stdint.h>

typedef enum {
  OP_ALLOC,
  OP_ASSIGN,
  OP_RET,

  OP_SEP, // set element pointer
  OP_GEP, // get element pointer

  OP_CALL,
  OP_PUSH_ARG,
  OP_POP_ARG,

  OP_PUSH,
  OP_POP,

  OP_LOAD,
  OP_LOAD_CONSTANT,
} Op_Code;

typedef struct {
  size_t address;
  size_t left;
  size_t right;
  size_t code;
} IR;

typedef struct {
  size_t address;
  union {
    int64_t number;
    String string;
  };
  enum {
    CONSTANT_NUMBER,
    CONSTANT_STRING,
  } type;
} IR_Constant;

typedef struct {
  struct { // list of all ir nodes.
    size_t length;
    size_t capacity;
    IR *data;
  };
  struct {
    size_t length;
    size_t capacity;
    IR_Constant *data;
  } constants;
} IR_Context;

static size_t ir_constant_push(IR_Context *ctx, IR_Constant constant) {
  if (ctx->constants.length >= ctx->constants.capacity) {
    ctx->constants.capacity =
        ctx->constants.capacity ? ctx->constants.capacity * 4 : 1;
    ctx->constants.data = realloc(ctx->constants.data, ctx->constants.capacity *
                                                           sizeof(IR_Constant));
  }
  ctx->constants.data[ctx->constants.length] = constant;
  return ctx->constants.length++;
}

static size_t embed_constant_number(IR_Context *ctx, int64_t number) {
  ir_constant_push(ctx, (IR_Constant){
                            .address = ctx->constants.length,
                            .number = number,
                            .type = CONSTANT_NUMBER,
                        });
  return ctx->constants.length - 1;
}

static size_t embed_constant_string(IR_Context *ctx, String string) {
  ir_constant_push(ctx, (IR_Constant){
                            .address = ctx->constants.length,
                            .string = string,
                            .type = CONSTANT_STRING,
                        });
  return ctx->constants.length - 1;
}

static inline void ir_push(IR_Context *data, IR ir) {
  if (data->length >= data->capacity) {
    data->capacity = data->capacity ? data->capacity * 4 : 1;
    data->data = realloc(data->data, data->capacity * sizeof(IR));
  }
  data->data[data->length++] = ir;
}

static size_t get_sizeof_block(AST *node) {
  assert(node->kind == AST_NODE_BLOCK && "cannot get stack size of non-block");
  size_t size = 0;
  for (int i = 0; i < node->statements.length; ++i) {
    auto statement = node->statements.data[i];
    if (statement->kind == AST_NODE_VARIABLE_DECLARATION) {
      size += calculate_sizeof_type(
          find_type(statement->variable_declaration.type));
    }
  }
  return size;
}

static void emit_ir(IR_Context *ctx, AST *node) {
  switch (node->kind) {
  case AST_NODE_PROGRAM: {
    return;
  } break;
  case AST_NODE_IDENTIFIER: {
    ir_push(ctx,
            (IR){
                .address = ctx->length,
                .code = OP_LOAD,
                .left = find_symbol(node->parent, node->identifier)->address,
            });
  } break;
  case AST_NODE_NUMBER: {
    ir_push(ctx,
            (IR){
                .address = ctx->length,
                .code = OP_LOAD_CONSTANT,
                .left = embed_constant_number(ctx, atoll(node->number.data)),
            });
  } break;
  case AST_NODE_STRING: {
    ir_push(ctx, (IR){
                     .address = ctx->length,
                     .code = OP_LOAD_CONSTANT,
                     .left = embed_constant_string(ctx, node->string),
                 });
  } break;
  case AST_NODE_FUNCTION_DECLARATION: {
    for (int i = 0; i < node->function_declaration.block->statements.length;
         ++i) {
      emit_ir(ctx, node->function_declaration.block->statements.data[i]);
    }
    ir_push(ctx, (IR){
                     .code = OP_RET,
                 });
  } break;
  case AST_NODE_VARIABLE_DECLARATION: {
    size_t variable_address = ctx->length;
    if (node->variable_declaration.default_value) {
      emit_ir(ctx, node->variable_declaration.default_value);
      ir_push(ctx, (IR){
                       .address = ctx->length,
                       .code = OP_ASSIGN,
                       .right = variable_address,
                   });
    }
    Symbol *symbol = find_symbol(node->parent, node->variable_declaration.name);
    symbol->address = variable_address;
  } break;

  case AST_NODE_ASSIGNMENT: {
    // TODO: IDK if this is gonna work.
    size_t value_address =
        find_symbol(node->parent, node->assignment.name)->address;
    emit_ir(ctx, node->assignment.right);
    ir_push(ctx, (IR){
                     .address = ctx->length,
                     .code = OP_ASSIGN,
                     .right = value_address,
                 });
  } break;
  case AST_NODE_FUNCTION_CALL: {
    auto symbol = find_symbol(node->parent, node->function_call.name);
    for (int i = 0; i < node->function_call.arguments_length; ++i) {
      emit_ir(ctx, node->function_call.arguments[i]);
      ir_push(ctx, (IR){
                       .address = ctx->length,
                       .code = OP_PUSH_ARG,
                   });
    }
    ir_push(
        ctx,
        (IR){
            .address = ctx->length,
            .code = OP_CALL,
            .left =
                find_symbol(node->parent, node->function_call.name)->address,
        });
    for (int i = 0; i < node->function_call.arguments_length; ++i) {
      ir_push(ctx, (IR){
                       .address = ctx->length,
                       .code = OP_POP_ARG,
                   });
    }
  } break;
  case AST_NODE_DOT_EXPRESSION: {
    Symbol *left = find_symbol(node->parent, node->dot_expression.left);
    size_t left_address = left->address;
    Type *left_type = left->type;
    size_t member_offset =
        calculate_member_offset(left_type, node->dot_expression.right);

    if (node->dot_expression.assignment_value) {
      size_t value_address = ctx->length;
      emit_ir(ctx, node->dot_expression.assignment_value);
      ir_push(ctx, (IR){
                       .address = ctx->length,
                       .code = OP_SEP,
                       .left = value_address,
                   });
    } else {
      ir_push(ctx, (IR){
                       .address = ctx->length,
                       .code = OP_GEP,
                       .left = left_address,
                       .right = member_offset,
                   });
    }
  } break;

  case AST_NODE_BLOCK: {
    for (int i = 0; i < node->statements.length; ++i) {
      auto statement = node->statements.data[i];
      emit_ir(ctx, statement);
    }
  } break;
  default:
    break;
  }
}

static void generate_ir(IR_Context *ctx, AST *entry_point) {
  if (entry_point->kind != AST_NODE_FUNCTION_DECLARATION ||
      !entry_point->function_declaration.is_entry) {
    panic("Error in generate_ir :: can only generate starting from the entry "
          "point of the program. the main function must be marked @entry");
  }
  auto decl = entry_point->function_declaration;

  for (int i = 0; i < decl.block->statements.length; ++i) {
    AST *statement = decl.block->statements.data[i];
    emit_ir(ctx, statement);
  }
}

#endif