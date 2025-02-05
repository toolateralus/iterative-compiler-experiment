#ifndef IR_H
#define IR_H
#include "core.h"
#include "parser.h"
#include "type.h"
#include <assert.h>
#include <stdint.h>

typedef struct {
  size_t address;
  size_t left;
  size_t right;
  enum {
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

    OP_BEGIN, // begin basic block
    OP_END,   // end basic block
  } code;
} IR_Inst;

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
  String name;
  AST_Parameter *parameters;
  size_t parameters_length;
  size_t address;
} IR_Extern;

typedef struct {
  struct { // list of all ir nodes.
    size_t length;
    size_t capacity;
    IR_Inst *data;
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
  struct {
    size_t length;
    size_t capacity;
    IR_Extern *data;
  } externs;
} IR_Context;


extern size_t address;

static size_t get_address_then_advance() {
  printf("address %zu\n", address);
  size_t result = address;
  (address)++;
  return result;
}

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
    ctx->types.capacity = ctx->types.capacity ? ctx->types.capacity * 4 : 1;
    ctx->types.data =
        realloc(ctx->types.data, ctx->types.capacity * sizeof(IR_Type));
  }
  ctx->types.data[ctx->types.length] = type;
  return ctx->types.length++;
}

static inline void ir_inst_push(IR_Context *data, IR_Inst ir) {
  if (data->length >= data->capacity) {
    data->capacity = data->capacity ? data->capacity * 4 : 1;
    data->data = realloc(data->data, data->capacity * sizeof(IR_Inst));
  }
  data->data[data->length++] = ir;
}

static inline void ir_extern_push(IR_Context* ctx, IR_Extern ir_extern) {
  if (ctx->externs.length >= ctx->externs.capacity) {
    ctx->externs.capacity = ctx->externs.capacity ? ctx->externs.capacity * 4 : 1;
    ctx->externs.data = realloc(ctx->externs.data, ctx->externs.capacity * sizeof(IR_Extern));
  }
  ctx->externs.data[ctx->externs.length++] = ir_extern;
}

static size_t get_size_of_block_allocations(AST *node) {
  assert(node->kind == AST_NODE_BLOCK && "cannot get stack size of non-block");
  size_t size = 0;
  for (int i = 0; i < node->statements.length; ++i) {
    auto statement = node->statements.data[i];
    if (statement->kind == AST_NODE_VARIABLE_DECLARATION) {
      size += find_type(statement->variable_declaration.type)->size;
    }
  }
  return size;
}

size_t emit_ir(IR_Context *ctx, AST *node);

static void write_ir_to_file(IR_Context *ctx, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (!file) {
    perror("Failed to open file");
    return;
  }

  fprintf(file, ".section :type:\n");
  for (size_t i = 0; i < ctx->types.length; ++i) {
    IR_Type *type = &ctx->types.data[i];
    fprintf(file, "  .type %s\n", type->name.data);
    for (size_t j = 0; j < type->members_length; ++j) {
      fprintf(file, "    .member %s, .offset %zu\n", type->members[j].type.data,
              type->members[j].offset);
    }
    fprintf(file, "  .end\n");
  }
  fprintf(file, ".end section\n\n");

  fprintf(file, ".section :constant:\n");
  for (size_t i = 0; i < ctx->constants.length; ++i) {
    IR_Constant *constant = &ctx->constants.data[i];
    switch (constant->type) {
    case CONSTANT_NUMBER:
      fprintf(file, "  .number @%zu, $%zu\n", constant->address,
              constant->number);
      break;
    case CONSTANT_STRING:
      fprintf(file, "  .string @%zu, $\"%s\"\n", constant->address,
              constant->string.data);
      break;
    }
  }

  fprintf(file, ".end section\n\n");
  fprintf(file, ".section :extern:\n");

  for (int i = 0; i < ctx->externs.length; ++i) {
    IR_Extern $extern = ctx->externs.data[i];
    fprintf(file, "  .function %s @%zu\n    .parameters (", $extern.name.data, $extern.address);
    for (int i = 0; i < $extern.parameters_length; ++i) {
      if ($extern.parameters[i].is_varargs) {
        fprintf(file, "...");
      } else {
        fprintf(file, "%s", $extern.parameters[i].type.data);
      }
      if (i != $extern.parameters_length -1) {
        fprintf(file, ", ");
      }
    }
    fprintf(file, ")\n  .end function\n");
  }


  fprintf(file, ".end section\n\n");
  fprintf(file, ".section :text:\n");

  int indent_level = 1;
  for (size_t i = 0; i < ctx->length; ++i) {
    IR_Inst *ir = &ctx->data[i];
    switch (ir->code) {
    case OP_ALLOC:
      fprintf(file, "%*s.alloc @%zu\n", indent_level * 2, "", ir->address);
      break;
    case OP_ASSIGN:
      fprintf(file, "%*s.assign %%%zu, %%%zu\n", indent_level * 2, "",
              ir->right, ir->left);
      break;
    case OP_RET:
      fprintf(file, "%*s.ret\n", indent_level * 2, "");
      break;
    case OP_SEP:
      fprintf(file, "%*s.sep @%zu, %%%zu, %%%zu\n", indent_level * 2, "",
              ir->address, ir->left, ir->right);
      break;
    case OP_GEP:
      fprintf(file, "%*s.gep @%zu, %%%zu, %%%zu\n", indent_level * 2, "",
              ir->address, ir->left, ir->right);
      break;
    case OP_CALL:
      fprintf(file, "%*s.call %%%zu\n", indent_level * 2, "", ir->left);
      break;
    case OP_PUSH_ARG:
      fprintf(file, "%*s.push_arg %%%zu\n", indent_level * 2, "", ir->left);
      break;
    case OP_POP_ARG:
      fprintf(file, "%*s.pop_arg %%%zu\n", indent_level * 2, "", ir->left);
      break;
    case OP_LOAD:
      fprintf(file, "%*s.load @%zu, %%%zu\n", indent_level * 2, "", ir->address,
              ir->left);
      break;
    case OP_LOAD_CONSTANT:
      fprintf(file, "%*s.load_constant @%zu, %%%zu\n", indent_level * 2, "",
              ir->address, ir->left);
      break;
    case OP_BEGIN:
      fprintf(file, "%*s.begin @%zu\n", indent_level * 2, "", ir->address);
      indent_level++;
      break;
    case OP_END:
      indent_level--;
      fprintf(file, "%*s.end @%zu\n", indent_level * 2, "", ir->address);
      break;
    default:
      fprintf(file, "%*sUNKNOWN_OP\n", indent_level * 2, "");
      break;
    }
  }
  fprintf(file, ".end section\n\n");

  fclose(file);
}

#endif