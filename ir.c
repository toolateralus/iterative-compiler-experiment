#include "ir.h"
#include "parser.h"

size_t emit_ir(IR_Context *ctx, AST *node) {
  switch (node->kind) {
  case AST_NODE_IDENTIFIER: {
    size_t address = get_address_then_advance();
    ir_inst_push(
        ctx, (IR_Inst){
                 .address = address,
                 .code = OP_LOAD,
                 .left = find_symbol(node->parent, node->identifier)->address,
             });
    return address;
  } break;
  case AST_NODE_NUMBER: {
    size_t address = get_address_then_advance();
    ir_inst_push(
        ctx, (IR_Inst){
                 .address = address,
                 .code = OP_LOAD_CONSTANT,
                 .left = embed_constant_number(ctx, atoll(node->number.data)),
             });
    return address;
  } break;
  case AST_NODE_STRING: {
    size_t address = get_address_then_advance();
    ir_inst_push(ctx, (IR_Inst){
                          .address = address,
                          .code = OP_LOAD_CONSTANT,
                          .left = embed_constant_string(ctx, node->string),
                      });
    return address;
  } break;
  case AST_NODE_VARIABLE_DECLARATION: {
    size_t variable_address = get_address_then_advance();
    if (node->variable_declaration.default_value) {
      ir_inst_push(ctx, (IR_Inst){
                            .address = variable_address,
                            .code = OP_ASSIGN,
                            .right = emit_ir(
                                ctx, node->variable_declaration.default_value),
                        });
    }
    Symbol *symbol = find_symbol(node->parent, node->variable_declaration.name);
    symbol->address = variable_address;
    return symbol->address;
  } break;
  case AST_NODE_ASSIGNMENT: {
    size_t left_address = get_address_then_advance();
    ir_inst_push(ctx, (IR_Inst){
                          .address = left_address,
                          .code = OP_ASSIGN,
                          .right = emit_ir(ctx, node->assignment.right),
                      });
    return 0;
  } break;
  case AST_NODE_FUNCTION_CALL: {
    auto symbol = find_symbol(node->parent, node->function_call.name);
    auto base_address = get_address_then_advance();
    for (int i = 0; i < node->function_call.arguments_length; ++i) {
      ir_inst_push(ctx, (IR_Inst){.address = ctx->length - base_address,
                                  .code = OP_PUSH_ARG,
                                  .left = emit_ir(
                                      ctx, node->function_call.arguments[i])});
    }
    ir_inst_push(ctx, (IR_Inst){
                          .address = base_address,
                          .code = OP_CALL,
                          .left = symbol->address,
                      });
    return base_address;
  } break;
  case AST_NODE_TYPE_DECLARATION: {
    IR_Type type = {
        .name = node->type_declaration.name,
    };
    for (size_t i = 0; i < node->type_declaration.members_length; ++i) {
      AST_Type_Member member = node->type_declaration.members[i];
      type.members[i].type = member.type;
      size_t offset = calculate_member_offset(
          find_type(node->type_declaration.name), member.name);
      type.members[i].offset = offset;
      type.members_length++;
    }
    ir_type_push(ctx, type);
    return 0;
  } break;
  case AST_NODE_DOT_EXPRESSION: {
    size_t address = get_address_then_advance();
    Symbol *left = find_symbol(node->parent, node->dot_expression.left);
    size_t left_address = left->address;
    Type *left_type = left->type;
    size_t member_offset =
        calculate_member_offset(left_type, node->dot_expression.right);

    if (node->dot_expression.assignment_value) {
      emit_ir(ctx, node->dot_expression.assignment_value);
      ir_inst_push(ctx, (IR_Inst){
                            .address = address,
                            .code = OP_SEP,
                            .left = left_address,
                            .right = member_offset,
                        });
    } else {
      ir_inst_push(ctx, (IR_Inst){
                            .address = address,
                            .code = OP_GEP,
                            .left = left_address,
                            .right = member_offset,
                        });
    }
    return address;
  } break;
  case AST_NODE_BLOCK: {
    size_t block_address = get_address_then_advance();
    size_t block_alloc_size = get_size_of_block_allocations(node);

    if (block_alloc_size != 0) {
      ir_inst_push(ctx, (IR_Inst){.code = OP_ALLOC,
                                  .address = block_address,
                                  .left = block_alloc_size});
    }
    for (int i = 0; i < node->statements.length; ++i) {
      emit_ir(ctx, node->statements.data[i]);
    }
    return block_address;
  } break;
  case AST_NODE_FUNCTION_DECLARATION: {
    if (node->function_declaration.is_extern) {
      return 0;
    }

    size_t function_address = get_address_then_advance();
    ir_inst_push(ctx, (IR_Inst){.code = OP_BEGIN, .address = function_address});
    emit_ir(ctx, node->function_declaration.block);
    ir_inst_push(ctx, (IR_Inst){.code = OP_RET});
    ir_inst_push(ctx, (IR_Inst){.code = OP_END, .address = function_address});
  } break;
  default:
    break;
  }

  return 0;
}
