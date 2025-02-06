#include "backend.h"
#include "core.h"
#include "lexer.h"
#include "parser.h"
#include "type.h"
#include <llvm-c/Core.h>
#include <llvm-c/DebugInfo.h>
#include <llvm-c/Error.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <llvm-c/Types.h>

/*
  TODO: maybe we want a string struct that's just null terminated constant.
  TODO: not sure.
  LLVMTypeRef string_type = LLVMStructCreateNamed(ctx->context, "String");
  LLVMTypeRef elements[] = {
    LLVMPointerType(LLVMIntType(8), 0),   // data,
    LLVMInt64Type(),                      // length
  };
  LLVMStructSetBody(string_type, elements, 2, 0); // last argument is bool:
  packed.
*/

LLVMTypeRef to_llvm_type(LLVM_Emit_Context *ctx, Type *type) {
  if (type->llvm_type)
    return type->llvm_type;
  switch (type->kind) {
  case VOID:
    return LLVMVoidType();
  case STRING:
    return LLVMPointerType(LLVMInt8Type(), 0);
  case I32:
    return LLVMInt32Type();

  // TODO: add option for packed struct?
  case STRUCT: {
    type->llvm_type = LLVMStructCreateNamed(ctx->context, type->name.data);

    LLVMTypeRef elements[type->members.length];
    ForEach(Type_Member, member, type->members,
            { elements[i] = to_llvm_type(ctx, member.type); });
    LLVMStructSetBody(type->llvm_type, elements, type->members.length, false);
    return type->llvm_type;
  };
  }
}

LLVMValueRef emit_program(LLVM_Emit_Context *ctx, AST *program) {
  ctx->context = LLVMContextCreate();
  ctx->builder = LLVMCreateBuilderInContext(ctx->context);
  ctx->module = LLVMModuleCreateWithNameInContext("program", ctx->context);
  ctx->debug_info = LLVMCreateDIBuilder(ctx->module);

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  char *target_triple = LLVMGetDefaultTargetTriple();
  char *features = LLVMGetHostCPUFeatures();
  char *cpu = LLVMGetHostCPUName();

  LLVMSetTarget(ctx->module, target_triple);

  LLVMTargetRef target;
  ctx->target = target;
  char *message;
  if (LLVMGetTargetFromTriple(target_triple, &target, &message)) {
    fprintf(stderr, "Error getting target from triple: %s\n", message);
    LLVMDisposeMessage(message);
    exit(1);
  }

  LLVMCodeGenOptLevel opt_level = LLVMCodeGenLevelAggressive;

  LLVMTargetMachineRef machine =
      LLVMCreateTargetMachine(target, target_triple, cpu, features, opt_level,
                              LLVMRelocPIC, LLVMCodeModelDefault);

  ctx->target_data = LLVMCreateTargetDataLayout(machine);

  for (int i = 0; i < program->statements.length; ++i) {
    AST *statement = program->statements.data[i];
    if (statement->kind == AST_NODE_TYPE_DECLARATION)
      emit_type_declaration(ctx, program->statements.data[i]);
  }

  for (int i = 0; i < program->statements.length; ++i) {
    AST *statement = program->statements.data[i];
    if (statement->kind == AST_NODE_FUNCTION_DECLARATION)
      emit_forward_declaration(ctx, program->statements.data[i]);
  }

  for (int i = 0; i < program->statements.length; ++i) {
    AST *statement = program->statements.data[i];
    emit_node(ctx, program->statements.data[i]);
  }

  if (COMPILATION_MODE == CM_RELEASE) {
    const char *passes = "default<O3>";
    LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();
    // LLVMPassBuilderOptionsSetVerifyEach(options, true); // Why can we not
    // verify? something is broken.
    LLVMErrorRef pass_error;
    if ((pass_error = LLVMRunPasses(ctx->module, passes, machine, options))) {
      char *message = LLVMGetErrorMessage(pass_error);
      fprintf(stderr, "Error running passes :: %s\n", message);
      ;
      exit(1);
    }
    LLVMDisposePassBuilderOptions(options);
  }

  char *error;
  if (LLVMPrintModuleToFile(ctx->module, "generated/output.ll", &error)) {
    printf("%s\n", error);
    exit(1);
  }

  LLVMDisposeMessage(target_triple);
  LLVMDisposeMessage(features);
  LLVMDisposeMessage(cpu);
  LLVMContextDispose(ctx->context);
  return nullptr;
}

LLVMValueRef emit_forward_declaration(LLVM_Emit_Context *ctx, AST *node) {
  LLVMTypeRef return_type = LLVMVoidTypeInContext(ctx->context);
  LLVMTypeRef *param_types = NULL;
  size_t parameters_length = node->function_declaration.parameters.length;
  bool is_varargs = false;

  if (parameters_length > 0) {
    param_types =
        (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * parameters_length);
    for (int i = 0; i < parameters_length; ++i) {
      AST_Parameter parameter =
          V_AT(AST_Parameter, node->function_declaration.parameters, i);
      if (parameter.is_varargs) {
        is_varargs = true;
        parameters_length--;
        break;
      } else {
        param_types[i] = to_llvm_type(ctx, find_type(parameter.type));
      }
    }
  }

  LLVMTypeRef function_type =
      LLVMFunctionType(return_type, param_types, parameters_length, is_varargs);
  LLVMValueRef function = LLVMAddFunction(
      ctx->module, node->function_declaration.name.data, function_type);

  Symbol *symbol = find_symbol(node, node->function_declaration.name);
  symbol->llvm_value = function;
  symbol->llvm_function_type = function_type;

  if (param_types) {
    free(param_types);
  }

  return function;
}

LLVMValueRef emit_function_declaration(LLVM_Emit_Context *ctx, AST *node) {
  if (!node->function_declaration.is_extern) {
    LLVMValueRef function =
        find_symbol(node, node->function_declaration.name)->llvm_value;
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(ctx->context, function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);
    emit_block(ctx, node->function_declaration.block);
    LLVMBuildRetVoid(ctx->builder);
  }
  return nullptr;
}

LLVMValueRef emit_function_call(LLVM_Emit_Context *ctx, AST *node) {
  Symbol *symbol = find_symbol(node, node->function_call.name);
  LLVMValueRef function = symbol->llvm_value;
  LLVMValueRef args[node->function_call.arguments.length];

  for (int i = 0; i < node->function_call.arguments.length; ++i) {
    LLVMValueRef arg =
        emit_node(ctx, V_AT(AST *, node->function_call.arguments, i));
    args[i] = arg;
  }

  return LLVMBuildCall2(ctx->builder, symbol->llvm_function_type, function,
                        args, node->function_call.arguments.length, "");
}

LLVMValueRef emit_type_declaration(LLVM_Emit_Context *ctx, AST *node) {
  to_llvm_type(ctx, find_type(node->type_declaration.name));
  return nullptr;
}

LLVMValueRef emit_identifier(LLVM_Emit_Context *ctx, AST *node) {
  Symbol *symbol = find_symbol(node, node->identifier);
  return LLVMBuildLoad2(ctx->builder, to_llvm_type(ctx, symbol->type),
                        symbol->llvm_value, symbol->name.data);
}
LLVMValueRef emit_number(LLVM_Emit_Context *ctx, AST *node) {
  return LLVMConstInt(LLVMInt32Type(), atoll(node->number.data), false);
}

LLVMValueRef emit_string(LLVM_Emit_Context *ctx, AST *node) {
  LLVMValueRef str =
      LLVMBuildGlobalString(ctx->builder, node->string.data, "str");
  return str;
}

LLVMValueRef emit_variable_declaration(LLVM_Emit_Context *ctx, AST *node) {
  LLVMTypeRef var_type =
      to_llvm_type(ctx, find_type(node->variable_declaration.type));
  LLVMValueRef var = LLVMBuildAlloca(ctx->builder, var_type,
                                     node->variable_declaration.name.data);
  find_symbol(node, node->variable_declaration.name)->llvm_value = var;
  if (node->variable_declaration.default_value) {
    LLVMValueRef init =
        emit_node(ctx, node->variable_declaration.default_value);
    LLVMBuildStore(ctx->builder, init, var);
  } else {
    LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
    LLVMValueRef size = LLVMSizeOf(var_type);
    unsigned alignment = LLVMABIAlignmentOfType(ctx->target_data, var_type);
    LLVMBuildMemSet(ctx->builder, var, zero, size, alignment);
  }
  return var;
}

LLVMValueRef emit_assignment(LLVM_Emit_Context *ctx, AST *node) {
  Symbol *symbol = find_symbol(node, node->assignment.name);
  LLVMValueRef value = emit_node(ctx, node->assignment.right);
  LLVMBuildStore(ctx->builder, value, symbol->llvm_value);
  return value;
}

LLVMValueRef emit_dot_expression(LLVM_Emit_Context *ctx, AST *node) {
  Symbol *base = find_symbol(node, node->dot_expression.left);
  Symbol *symbol = find_symbol(node, node->dot_expression.right);
  LLVMValueRef gep = LLVMBuildStructGEP2(
      ctx->builder, to_llvm_type(ctx, base->type), base->llvm_value,
      get_member_index(base->type, node->dot_expression.right), "dot_expr");

  if (node->dot_expression.assignment_value) {
    LLVMValueRef value = emit_node(ctx, node->dot_expression.assignment_value);
    LLVMBuildStore(ctx->builder, value, gep);
    return value;
  } else {
    return LLVMBuildLoad2(ctx->builder, LLVMPointerType(to_llvm_type(ctx, node->type), 0), gep, "load_dot_expr");
  }
}

LLVMValueRef emit_block(LLVM_Emit_Context *ctx, AST *node) {
  for (int i = 0; i < node->statements.length; ++i) {
    emit_node(ctx, node->statements.data[i]);
  }
  return nullptr;
}

LLVMValueRef emit_node(LLVM_Emit_Context *ctx, AST *node) {
  switch (node->kind) {
  case AST_NODE_PROGRAM:
    return emit_program(ctx, node);
  case AST_NODE_IDENTIFIER:
    return emit_identifier(ctx, node);
  case AST_NODE_NUMBER:
    return emit_number(ctx, node);
  case AST_NODE_STRING:
    return emit_string(ctx, node);
  case AST_NODE_FUNCTION_DECLARATION:
    return emit_function_declaration(ctx, node);
  case AST_NODE_TYPE_DECLARATION:
    return emit_type_declaration(ctx, node);
  case AST_NODE_VARIABLE_DECLARATION:
    return emit_variable_declaration(ctx, node);
  case AST_NODE_ASSIGNMENT:
    return emit_assignment(ctx, node);
  case AST_NODE_DOT_EXPRESSION:
    return emit_dot_expression(ctx, node);
  case AST_NODE_FUNCTION_CALL:
    return emit_function_call(ctx, node);
  case AST_NODE_BLOCK:
    return emit_block(ctx, node);
  case AST_NODE_BINARY_EXPRESSION:
    return emit_binary_expression(ctx, node);
  default:
    fprintf(stderr, "Unknown AST node kind: %d\n", node->kind);
    exit(1);
  }
}

LLVMValueRef emit_binary_expression(LLVM_Emit_Context *ctx, AST *node) {
  LLVMValueRef left = emit_node(ctx, node->binary_expression.left);
  LLVMValueRef right = emit_node(ctx, node->binary_expression.right);

  switch (node->binary_expression.operator) {
  case TOKEN_ADD:
    return LLVMBuildAdd(ctx->builder, left, right, "addtmp");
  case TOKEN_SUB:
    return LLVMBuildSub(ctx->builder, left, right, "subtmp");
  case TOKEN_MUL:
    return LLVMBuildMul(ctx->builder, left, right, "multmp");
  case TOKEN_DIV:
    return LLVMBuildSDiv(ctx->builder, left, right, "divtmp");
  case TOKEN_MOD:
    return LLVMBuildSRem(ctx->builder, left, right, "modtmp");
  case TOKEN_AND:
    return LLVMBuildAnd(ctx->builder, left, right, "andtmp");
  case TOKEN_OR:
    return LLVMBuildOr(ctx->builder, left, right, "ortmp");
  case TOKEN_XOR:
    return LLVMBuildXor(ctx->builder, left, right, "xortmp");
  case TOKEN_SHL:
    return LLVMBuildShl(ctx->builder, left, right, "shltmp");
  case TOKEN_SHR:
    return LLVMBuildLShr(ctx->builder, left, right, "shrtmp");
  case TOKEN_EQ:
    return LLVMBuildICmp(ctx->builder, LLVMIntEQ, left, right, "eqtmp");
  case TOKEN_NEQ:
    return LLVMBuildICmp(ctx->builder, LLVMIntNE, left, right, "neqtmp");
  case TOKEN_LT:
    return LLVMBuildICmp(ctx->builder, LLVMIntSLT, left, right, "lttmp");
  case TOKEN_GT:
    return LLVMBuildICmp(ctx->builder, LLVMIntSGT, left, right, "gttmp");
  case TOKEN_LTE:
    return LLVMBuildICmp(ctx->builder, LLVMIntSLE, left, right, "ltetmp");
  case TOKEN_GTE:
    return LLVMBuildICmp(ctx->builder, LLVMIntSGE, left, right, "gtetmp");
  default:
    fprintf(stderr, "Unknown binary operator: %d\n",
            node->binary_expression.operator);
    exit(1);
  }
}
