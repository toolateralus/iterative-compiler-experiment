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

static void print_type(LLVMTypeRef type) {
  char *llvm_type_str = LLVMPrintTypeToString(type);
  printf("LLVM Type: %s\n", llvm_type_str);
  LLVMDisposeMessage(llvm_type_str);
}

static void print_value(LLVMValueRef value) {
  char *value_str = LLVMPrintValueToString(value);
  printf("%s\n", value_str);
  LLVMDisposeMessage(value_str);
}

// Be very careful using this macro, it can't have do { } while(0) semantic because
// it's very likely to be used to declare some variables.
#define DONT_LOAD(old_state, ctx, block)                                                  \
  bool old_state = ctx->dont_load;\
  ctx->dont_load = true;                                                       \
  block                                                                        \
  ctx->dont_load = old_state;

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
    LLVMTypeRef elements[type->$struct.members.length];
    ForEach(Type_Member, member, type->$struct.members, {
      elements[i] = to_llvm_type(ctx, V_PTR_AT(Type, type_table, member.type));
    });
    LLVMStructSetBody(type->llvm_type, elements, type->$struct.members.length,
                      false);
    return type->llvm_type;
  };
  case FUNCTION: {
    LLVMTypeRef return_type =
        to_llvm_type(ctx, get_type(type->$function.$return));
    LLVMTypeRef param_types[type->$function.parameters.length];
    for (int i = 0; i < type->$function.parameters.length; ++i) {
      param_types[i] = to_llvm_type(ctx, V_PTR_AT(Type, type_table, i));
    }
    type->llvm_type = LLVMFunctionType(
        return_type, param_types, type->$function.parameters.length, false);
    return type->llvm_type;
  }
  case F32:
    return LLVMFloatType();
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

  { // GENERATE THE CODE BY VISITING THE AST
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
  LLVMTypeRef return_type =
      to_llvm_type(ctx, find_type(node->function.return_type));
  LLVMTypeRef *param_types = NULL;
  size_t parameters_length = node->function.parameters.length;
  bool is_varargs = false;

  if (parameters_length > 0) {
    param_types =
        (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * parameters_length);
    for (int i = 0; i < parameters_length; ++i) {
      AST_Parameter parameter =
          V_AT(AST_Parameter, node->function.parameters, i);
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
  LLVMValueRef function =
      LLVMAddFunction(ctx->module, node->function.name.data, function_type);

  Symbol *symbol = find_symbol(node, node->function.name);
  symbol->llvm_value = function;
  symbol->llvm_function_type = function_type;

  if (param_types) {
    free(param_types);
  }

  return function;
}

LLVMValueRef emit_function_declaration(LLVM_Emit_Context *ctx, AST *node) {
  if (!node->function.is_extern) {
    LLVMValueRef function = find_symbol(node, node->function.name)->llvm_value;
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(ctx->context, function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);
    emit_block(ctx, node->function.block);

    if (strncmp(node->function.return_type.data, "void", 4) == 0) {
      LLVMBuildRetVoid(ctx->builder);
    }
  }
  return nullptr;
}

LLVMValueRef emit_function_call(LLVM_Emit_Context *ctx, AST *node) {
  Symbol *symbol = find_symbol(node, node->call.name);
  LLVMValueRef function = symbol->llvm_value;
  LLVMValueRef args[node->call.arguments.length];

  for (int i = 0; i < node->call.arguments.length; ++i) {
    LLVMValueRef arg = emit_node(ctx, V_AT(AST *, node->call.arguments, i));
    args[i] = arg;
  }

  return LLVMBuildCall2(ctx->builder, symbol->llvm_function_type, function,
                        args, node->call.arguments.length, "");
}

LLVMValueRef emit_type_declaration(LLVM_Emit_Context *ctx, AST *node) {
  to_llvm_type(ctx, find_type(node->declaration.name));
  return nullptr;
}

LLVMValueRef emit_identifier(LLVM_Emit_Context *ctx, AST *node) {
  Symbol *symbol = find_symbol(node, node->identifier);

  auto llvm_type = to_llvm_type(ctx, get_type(symbol->type));

  LLVMValueRef value = nullptr;

  // TODO: we should probably just do this when we emit a function, not every
  // time we need to look up a parameter.
  if (symbol->llvm_value == nullptr) {
    LLVMValueRef function =
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    unsigned param_index = get_parameter_index(symbol->node, node->identifier);
    return LLVMGetParam(function, param_index);
  } else {
    value = symbol->llvm_value;
  }

  if (ctx->dont_load) {
    return value;
  } else {
    return LLVMBuildLoad2(ctx->builder, llvm_type, value, symbol->name.data);
  }
}

LLVMValueRef emit_number(LLVM_Emit_Context *ctx, AST *node) {
  return LLVMConstInt(LLVMInt32Type(), atoll(node->number.data), false);
}

// for making string literals with '\escape chars' work.
static inline char *unescape_string_lit(const char *s) {
  size_t len = strlen(s);
  char *res = (char *)malloc(len + 1); // Allocate memory for the result
  char *dst = res;
  const char *src = s;

  while (*src) {
    if (*src == '\\') {
      src++;
      switch (*src) {
      case 'e':
        *dst++ = '\x1B';
        break;
      case 'n':
        *dst++ = '\n';
        break;
      case 't':
        *dst++ = '\t';
        break;
      case 'r':
        *dst++ = '\r';
        break;
      case 'f':
        *dst++ = '\f';
        break;
      case 'v':
        *dst++ = '\v';
        break;
      case 'a':
        *dst++ = '\a';
        break;
      case 'b':
        *dst++ = '\b';
        break;
      case '\\':
        *dst++ = '\\';
        break;
      case '\'':
        *dst++ = '\'';
        break;
      case '\"':
        *dst++ = '\"';
        break;
      case '\?':
        *dst++ = '\?';
        break;
      case '0':
        *dst++ = '\0';
        break;
      default:
        // If the character following the backslash is not a recognized escape
        // sequence, append the backslash followed by the character itself.
        *dst++ = '\\';
        *dst++ = *src;
        break;
      }
    } else {
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0'; // Null-terminate the result
  return res;
}

LLVMValueRef emit_string(LLVM_Emit_Context *ctx, AST *node) {
  char *encoded_str = unescape_string_lit(node->string.data);
  LLVMValueRef result = LLVMBuildGlobalString(ctx->builder, encoded_str, "str");
  free(encoded_str);
  return result;
}

LLVMValueRef emit_variable_declaration(LLVM_Emit_Context *ctx, AST *node) {
  LLVMTypeRef var_type = to_llvm_type(ctx, find_type(node->variable.type));
  LLVMValueRef var =
      LLVMBuildAlloca(ctx->builder, var_type, node->variable.name.data);
  find_symbol(node, node->variable.name)->llvm_value = var;
  if (node->variable.default_value) {
    LLVMValueRef init = emit_node(ctx, node->variable.default_value);
    LLVMBuildStore(ctx->builder, init, var);
  } else {
    // TODO: figure out why memset refuses to link.
    // TODO; we should have zero-initialization-by-default;
    // LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, false);
    // LLVMValueRef size = LLVMSizeOf(var_type);
    // unsigned alignment = LLVMABIAlignmentOfType(ctx->target_data, var_type);
    // LLVMBuildMemSet(ctx->builder, var, zero, size, alignment);
  }
  return var;
}

LLVMValueRef emit_dot_expression(LLVM_Emit_Context *ctx, AST *node) {
  DONT_LOAD(old_state, ctx, 
    LLVMValueRef left = emit_node(ctx, node->dot.left);
  )
  Type *left_type = get_type(node->dot.left->type);
  size_t member_index = get_member_index(left_type, node->dot.member_name);
  Type *member_type = get_type(V_AT(Type_Member, left_type->$struct.members, member_index).type);
  LLVMValueRef gep =
      LLVMBuildStructGEP2(ctx->builder, to_llvm_type(ctx, left_type), left,
                          member_index, "dotexpr");

  // For assignment.
  if (ctx->dont_load) {
    return gep;
  } else {
    LLVMTypeRef llvm_member_type = to_llvm_type(ctx, member_type);
    return LLVMBuildLoad2(ctx->builder, llvm_member_type, gep, "load_dot_expr");
  }
}



LLVMValueRef emit_binary_expression(LLVM_Emit_Context *ctx, AST *node) {
  if (node->binary.operator == TOKEN_ASSIGN) {
    DONT_LOAD(old_state, ctx, 
      LLVMValueRef left = emit_node(ctx, node->binary.left);
    )    
    LLVMValueRef right = emit_node(ctx, node->binary.right);
    LLVMBuildStore(ctx->builder, right, left);
    return nullptr;
  }

  LLVMValueRef left = emit_node(ctx, node->binary.left);
  LLVMValueRef right = emit_node(ctx, node->binary.right);

  switch (node->binary.operator) {
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
    fprintf(stderr, "Unknown binary operator: %d\n", node->binary.operator);
    exit(1);
  }
}

LLVMValueRef emit_return_statement(LLVM_Emit_Context *ctx, AST *node) {
  if (node->$return) {
    LLVMBuildRet(ctx->builder, emit_node(ctx, node->$return));
  } else {
    LLVMBuildRetVoid(ctx->builder);
  }
  return nullptr;
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
  case AST_NODE_DOT_EXPRESSION:
    return emit_dot_expression(ctx, node);
  case AST_NODE_FUNCTION_CALL:
    return emit_function_call(ctx, node);
  case AST_NODE_BLOCK:
    return emit_block(ctx, node);
  case AST_NODE_BINARY_EXPRESSION:
    return emit_binary_expression(ctx, node);
  case AST_NODE_RETURN:
    return emit_return_statement(ctx, node);
  default:
    fprintf(stderr, "Unknown AST node kind: %d\n", node->kind);
    exit(1);
  }
}
