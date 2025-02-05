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
  String name;
  struct {
    String type;
    int64_t offset;
  } members[4];
  size_t members_length;
} IR_Type;

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
  struct {
    size_t length;
    size_t capacity;
    IR_Type *data;
  } types;
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

static size_t ir_type_push(IR_Context *ctx, IR_Type type) {
  if (ctx->types.length >= ctx->types.capacity) {
    ctx->types.capacity =
        ctx->types.capacity ? ctx->types.capacity * 4 : 1;
    ctx->types.data = realloc(ctx->types.data, ctx->types.capacity *
                                                       sizeof(IR_Type));
  }
  ctx->types.data[ctx->types.length] = type;
  return ctx->types.length++;
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

typedef struct {
  AST *node;
  bool emitted;
} FunctionEntry;

typedef struct {
  size_t length;
  size_t capacity;
  FunctionEntry *data;
} FunctionTable;

static void function_table_push(FunctionTable *table, AST *node) {
  if (table->length >= table->capacity) {
    table->capacity = table->capacity ? table->capacity * 2 : 1;
    table->data = realloc(table->data, table->capacity * sizeof(FunctionEntry));
  }
  table->data[table->length++] =
      (FunctionEntry){.node = node, .emitted = false};
}

static FunctionEntry *find_function(FunctionTable *table, String name) {
  for (size_t i = 0; i < table->length; ++i) {
    if (Strings_compare(table->data[i].node->function_declaration.name, name)) {
      return &table->data[i];
    }
  }
  return NULL;
}

static void collect_functions(FunctionTable *table, AST *node) {
  for (int i = 0; i < node->statements.length; ++i) {
    auto statement = node->statements.data[i];
    switch (statement->kind) {
    case AST_NODE_FUNCTION_DECLARATION:
      function_table_push(table, statement);
    default:
      break;
    }
  }
}

static void emit_ir(IR_Context *ctx, AST *node, FunctionTable *table);

static void emit_function_ir(IR_Context *ctx, FunctionEntry *entry,
                             FunctionTable *table) {
  if (entry->emitted || entry->node->kind != AST_NODE_FUNCTION_DECLARATION ||
      entry->node->function_declaration.is_extern)
    return;

  entry->emitted = true;

  AST *node = entry->node;
  for (int i = 0; i < node->function_declaration.block->statements.length;
       ++i) {
    AST *statement = node->function_declaration.block->statements.data[i];
    emit_ir(ctx, statement, table);
  }

  ir_push(ctx, (IR){.code = OP_RET});
}

static void emit_ir(IR_Context *ctx, AST *node, FunctionTable *table) {
  switch (node->kind) {
  case AST_NODE_PROGRAM: {
    for (int i = 0; i < node->statements.length; ++i) {
      emit_ir(ctx, node->statements.data[i], table);
    }
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
  case AST_NODE_VARIABLE_DECLARATION: {
    size_t variable_address = ctx->length;
    if (node->variable_declaration.default_value) {
      emit_ir(ctx, node->variable_declaration.default_value, table);
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
    size_t value_address = ctx->length;
    emit_ir(ctx, node->assignment.right, table);
    ir_push(ctx, (IR){
                     .address = ctx->length,
                     .code = OP_ASSIGN,
                     .right = value_address,
                 });
  } break;
  case AST_NODE_FUNCTION_CALL: {
    auto symbol = find_symbol(node->parent, node->function_call.name);
    FunctionEntry *entry = find_function(table, node->function_call.name);
    if (entry) {
      emit_function_ir(ctx, entry, table);
    }
    for (int i = 0; i < node->function_call.arguments_length; ++i) {
      emit_ir(ctx, node->function_call.arguments[i], table);
      ir_push(ctx, (IR){
                       .address = ctx->length,
                       .code = OP_PUSH_ARG,
                   });
    }
    ir_push(ctx, (IR){
                     .address = ctx->length,
                     .code = OP_CALL,
                     .left = symbol->address,
                 });
    for (int i = 0; i < node->function_call.arguments_length; ++i) {
      ir_push(ctx, (IR){
                       .address = ctx->length,
                       .code = OP_POP_ARG,
                   });
    }
  } break;
  case AST_NODE_TYPE_DECLARATION: {
    IR_Type type = {
        .name = node->type_declaration.name,
    };
    for (size_t i = 0; i < node->type_declaration.members_length; ++i) {
      AST_Type_Member member = node->type_declaration.members[i];
      type.members[i].type = member.type;
      type.members[i].offset = calculate_member_offset(find_type(member.type), member.name);
      type.members_length++;
    }
    ir_type_push(ctx, type);
  } break;
  case AST_NODE_DOT_EXPRESSION: {
    Symbol *left = find_symbol(node->parent, node->dot_expression.left);
    size_t left_address = left->address;
    Type *left_type = left->type;
    size_t member_offset =
        calculate_member_offset(left_type, node->dot_expression.right);

    if (node->dot_expression.assignment_value) {
      size_t value_address = ctx->length;
      emit_ir(ctx, node->dot_expression.assignment_value, table);
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
      emit_ir(ctx, node->statements.data[i], table);
    }
  } break;
  default:
    break;
  }
}

static void generate_ir(IR_Context *ctx, AST *program) {
  FunctionTable function_table = {0};

  for (int i = 0; i < program->statements.length; ++i) {
    AST *statement = program->statements.data[i];
    if (statement->kind == AST_NODE_TYPE_DECLARATION) {
      emit_ir(ctx, statement, &function_table);
    }
  }

  collect_functions(&function_table, program);
  for (int i = 0; i < function_table.length; ++i) {
    emit_function_ir(ctx, &function_table.data[i], &function_table);
  }
}

static void write_ir_to_file(IR_Context *ctx, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    perror("Failed to open file");
    return;
  }

  // Emit types
  for (size_t i = 0; i < ctx->types.length; ++i) {
    IR_Type *type = &ctx->types.data[i];
    fprintf(file, "TYPE %s\n", type->name.data);
    for (size_t j = 0; j < type->members_length; ++j) {
      fprintf(file, "  MEMBER %s, OFFSET %zu\n",
              type->members[j].type.data, type->members[j].offset);
    }
  }

  // Emit constants
  for (size_t i = 0; i < ctx->constants.length; ++i) {
    IR_Constant *constant = &ctx->constants.data[i];
    switch (constant->type) {
    case CONSTANT_NUMBER:
      fprintf(file, "CONSTANT_NUMBER %zu, %zu\n", constant->address,
              constant->number);
      break;
    case CONSTANT_STRING:
      fprintf(file, "CONSTANT_STRING %zu, \"%s\"\n", constant->address,
              constant->string.data);
      break;
    }
  }

  // Emit instructions
  for (size_t i = 0; i < ctx->length; ++i) {
    IR *ir = &ctx->data[i];
    switch (ir->code) {
    case OP_ALLOC:
      fprintf(file, "ALLOC %zu\n", ir->address);
      break;
    case OP_ASSIGN:
      fprintf(file, "ASSIGN %zu, %zu\n", ir->right, ir->left);
      break;
    case OP_RET:
      fprintf(file, "RET\n");
      break;
    case OP_SEP:
      fprintf(file, "SEP %zu, %zu, %zu\n", ir->address, ir->left, ir->right);
      break;
    case OP_GEP:
      fprintf(file, "GEP %zu, %zu, %zu\n", ir->address, ir->left, ir->right);
      break;
    case OP_CALL:
      fprintf(file, "CALL %zu\n", ir->left);
      break;
    case OP_PUSH_ARG:
      fprintf(file, "PUSH_ARG %zu\n", ir->left);
      break;
    case OP_POP_ARG:
      fprintf(file, "POP_ARG %zu\n", ir->left);
      break;
    case OP_LOAD:
      fprintf(file, "LOAD %zu, %zu\n", ir->address, ir->left);
      break;
    case OP_LOAD_CONSTANT:
      fprintf(file, "LOAD_CONSTANT %zu, %zu\n", ir->address, ir->left);
      break;
    default:
      fprintf(file, "UNKNOWN_OP\n");
      break;
    }
  }

  fclose(file);
}

#endif