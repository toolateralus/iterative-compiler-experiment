#include "backend.h"
#include "core.h"
#include "lexer.h"
#include "thir.h"
#include "type.h"
#include <llvm-c/Core.h>
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

static inline char *unescape_string_lit(const char *s) {
  size_t len = strlen(s);
  char *res = (char *)malloc(len + 1);
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
          *dst++ = '\\';
          *dst++ = *src;
          break;
      }
    } else {
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0';
  return res;
}

#define DONT_LOAD(old_state, ctx, block) \
  bool old_state = ctx->dont_load;       \
  ctx->dont_load = true;                 \
  block ctx->dont_load = old_state;

LLVMTypeRef to_llvm_type(LLVM_Emit_Context *ctx, Type *type) {
  if (type->llvm_type) return type->llvm_type;
  switch (type->kind) {
    case VOID: {
      static LLVMTypeRef type;
      if (!type) type = LLVMVoidType();
      return type;
    }
    case STRING: {
      static LLVMTypeRef type;
      if (!type) type = LLVMPointerType(LLVMInt8Type(), 0);
      return type;
    }
    case I32: {
      static LLVMTypeRef type;
      if (!type) type = LLVMInt32Type();
      return type;
    }
    case STRUCT: {
      type->llvm_type = LLVMStructCreateNamed(ctx->context, type->name.data);
      LLVMTypeRef elements[type->$struct.members.length];
      ForEach(Type_Member, member, type->$struct.members,
              { elements[i] = to_llvm_type(ctx, V_PTR_AT(Type, type_table, member.type)); });
      LLVMStructSetBody(type->llvm_type, elements, type->$struct.members.length, false);
      return type->llvm_type;
    }
    case FUNCTION: {
      LLVMTypeRef return_type = to_llvm_type(ctx, get_type(type->$function.$return));
      size_t params_size = type->$function.parameters.length;

      if (type->$function.is_varargs) {
        params_size--;
      }

      LLVMTypeRef param_types[params_size];
      for (int i = 0; i < type->$function.parameters.length; ++i) {
        param_types[i] = to_llvm_type(ctx, get_type(V_AT(size_t, type->$function.parameters, i)));
      }

      type->llvm_type = LLVMFunctionType(return_type, param_types, type->$function.parameters.length, false);
      return type->llvm_type;
    }
    case F32:
      return LLVMFloatType();
  }
}

LLVMValueRef emit_thir_node(LLVM_Emit_Context *ctx, THIR *node);

LLVMValueRef emit_thir_program(LLVM_Emit_Context *ctx, THIR *program) {
  ctx->context = LLVMContextCreate();
  ctx->builder = LLVMCreateBuilderInContext(ctx->context);
  ctx->module = LLVMModuleCreateWithNameInContext("program", ctx->context);

  LLVMInitializeNativeTarget();

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

  LLVMCodeGenOptLevel opt_level = LLVMCodeGenLevelDefault;

  LLVMTargetMachineRef machine =
      LLVMCreateTargetMachine(target, target_triple, cpu, features, opt_level, LLVMRelocPIC, LLVMCodeModelDefault);

  ctx->target_data = LLVMCreateTargetDataLayout(machine);

  for (size_t i = 0; i < program->statements.length; ++i) {
    THIR *node = program->statements.nodes[i];
    if (node->kind == THIR_FUNCTION && node->function.is_entry) {
      emit_thir_node(ctx, node);
      break;
    }
  }

  if (COMPILATION_MODE == CM_RELEASE) {
    const char *passes = "default<O3>";
    LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();
    LLVMErrorRef pass_error;
    if ((pass_error = LLVMRunPasses(ctx->module, passes, machine, options))) {
      char *message = LLVMGetErrorMessage(pass_error);
      fprintf(stderr, "Error running passes :: %s\n", message);
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

LLVMValueRef emit_thir_function_forward_declaration(LLVM_Emit_Context *ctx, THIR *node) {
  if (node->function.llvm_function) {
    return node->function.llvm_function;
  }
  Type *fn_ty = get_type(node->type);
  LLVMTypeRef return_type = to_llvm_type(ctx, get_type(fn_ty->$function.$return));
  LLVMTypeRef *param_types = NULL;
  size_t parameters_length = node->function.parameters.length;
  bool is_varargs = false;

  if (parameters_length > 0) {
    param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * parameters_length);
    for (int i = 0; i < parameters_length; ++i) {
      THIRParameter parameter = V_AT(THIRParameter, node->function.parameters, i);
      if (parameter.is_vararg) {
        is_varargs = true;
        parameters_length--;
        break;
      } else {
        param_types[i] = to_llvm_type(ctx, get_type(parameter.type));
      }
    }
  }

  LLVMTypeRef function_type = LLVMFunctionType(return_type, param_types, parameters_length, is_varargs);

  LLVMValueRef function = LLVMGetNamedFunction(ctx->module, node->function.name.data);
  if (!function) {
    function = LLVMAddFunction(ctx->module, node->function.name.data, function_type);
  }
  
  node->function.llvm_function = function;

  if (param_types) {
    free(param_types);
  }

  return function;
}

LLVMValueRef emit_thir_function(LLVM_Emit_Context *ctx, THIR *node) {

  LLVMValueRef function = emit_thir_function_forward_declaration(ctx, node);

  if (node->function.is_extern) {
    return function;
  }

  LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx->context, function, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, entry);
  emit_thir_node(ctx, node->function.block);
  Type *fn_type = get_type(node->type);
  if (fn_type->$function.$return == VOID) {
    LLVMBuildRetVoid(ctx->builder);
  }
  node->function.llvm_function = function;
  return nullptr;
}

LLVMValueRef emit_thir_type_declaration(LLVM_Emit_Context *ctx, THIR *node) {
  to_llvm_type(ctx, get_type(node->type));
  return NULL;
}

LLVMValueRef emit_thir_variable_declaration(LLVM_Emit_Context *ctx, THIR *node) {
  LLVMTypeRef var_type = to_llvm_type(ctx, get_type(node->type));
  LLVMValueRef var = LLVMBuildAlloca(ctx->builder, var_type, node->variable.name.data);
  // TODO: Store LLVMValueRef in your symbol table for this variable

  if (node->variable.value) {
    LLVMValueRef init = emit_thir_node(ctx, node->variable.value);
    LLVMBuildStore(ctx->builder, init, var);
  }
  return var;
}

LLVMValueRef emit_thir_identifier(LLVM_Emit_Context *ctx, THIR *node) {
  // TODO: Lookup symbol in your THIR symbol table
  LLVMValueRef value = emit_thir_node(ctx, node->identifier.resolved);
  LLVMTypeRef llvm_type = to_llvm_type(ctx, get_type(node->type));
  if (ctx->dont_load) {
    return value;
  } else {
    return LLVMBuildLoad2(ctx->builder, llvm_type, value, node->identifier.name.data);
  }
}

LLVMValueRef emit_thir_number(LLVM_Emit_Context *ctx, THIR *node) {
  return LLVMConstInt(LLVMInt32Type(), atoll(node->number.data), false);
}

LLVMValueRef emit_thir_string(LLVM_Emit_Context *ctx, THIR *node) {
  char *encoded_str = unescape_string_lit(node->string.data);
  LLVMValueRef result = LLVMBuildGlobalString(ctx->builder, encoded_str, "str");
  free(encoded_str);
  return result;
}

LLVMValueRef emit_thir_member_access(LLVM_Emit_Context *ctx, THIR *node) {
  DONT_LOAD(old_state, ctx, LLVMValueRef left = emit_thir_node(ctx, node->member_access.base);)
  Type *left_type = get_type(node->member_access.base->type);
  size_t member_index = get_member_index(left_type, node->member_access.member);
  Type *member_type = get_type(V_AT(Type_Member, left_type->$struct.members, member_index).type);
  LLVMValueRef gep = LLVMBuildStructGEP2(ctx->builder, to_llvm_type(ctx, left_type), left, member_index, "dotexpr");

  if (ctx->dont_load) {
    return gep;
  } else {
    LLVMTypeRef llvm_member_type = to_llvm_type(ctx, member_type);
    LLVMValueRef load = LLVMBuildLoad2(ctx->builder, llvm_member_type, gep, "load_dot_expr");
    return load;
  }
}

LLVMValueRef emit_thir_call(LLVM_Emit_Context *ctx, THIR *node) {
  LLVMValueRef function = emit_thir_node(ctx, node->call.function);

  size_t argc = node->call.arguments.length;
  LLVMValueRef args[argc];
  for (size_t i = 0; i < argc; ++i) {
    args[i] = emit_thir_node(ctx, node->call.arguments.nodes[i]);
  }
  LLVMTypeRef fn_type = to_llvm_type(ctx, get_type(node->call.function->type));
  LLVMValueRef call = LLVMBuildCall2(ctx->builder, fn_type, function, args, argc, "");
  return call;
}

LLVMValueRef emit_thir_binary_expression(LLVM_Emit_Context *ctx, THIR *node) {
  if (node->binary.operator== TOKEN_ASSIGN) {
    DONT_LOAD(old_state, ctx, LLVMValueRef left = emit_thir_node(ctx, node->binary.left);)
    LLVMValueRef right = emit_thir_node(ctx, node->binary.right);
    LLVMBuildStore(ctx->builder, right, left);
    return nullptr;
  }

  LLVMValueRef left = emit_thir_node(ctx, node->binary.left);
  LLVMValueRef right = emit_thir_node(ctx, node->binary.right);

  LLVMValueRef result;
  switch (node->binary.operator) {
    case TOKEN_ADD:
      result = LLVMBuildAdd(ctx->builder, left, right, "addtmp");
      break;
    case TOKEN_SUB:
      result = LLVMBuildSub(ctx->builder, left, right, "subtmp");
      break;
    case TOKEN_MUL:
      result = LLVMBuildMul(ctx->builder, left, right, "multmp");
      break;
    case TOKEN_DIV:
      result = LLVMBuildSDiv(ctx->builder, left, right, "divtmp");
      break;
    case TOKEN_MOD:
      result = LLVMBuildSRem(ctx->builder, left, right, "modtmp");
      break;
    case TOKEN_AND:
      result = LLVMBuildAnd(ctx->builder, left, right, "andtmp");
      break;
    case TOKEN_OR:
      result = LLVMBuildOr(ctx->builder, left, right, "ortmp");
      break;
    case TOKEN_XOR:
      result = LLVMBuildXor(ctx->builder, left, right, "xortmp");
      break;
    case TOKEN_SHL:
      result = LLVMBuildShl(ctx->builder, left, right, "shltmp");
      break;
    case TOKEN_SHR:
      result = LLVMBuildLShr(ctx->builder, left, right, "shrtmp");
      break;
    case TOKEN_EQ:
      result = LLVMBuildICmp(ctx->builder, LLVMIntEQ, left, right, "eqtmp");
      break;
    case TOKEN_NEQ:
      result = LLVMBuildICmp(ctx->builder, LLVMIntNE, left, right, "neqtmp");
      break;
    case TOKEN_LT:
      result = LLVMBuildICmp(ctx->builder, LLVMIntSLT, left, right, "lttmp");
      break;
    case TOKEN_GT:
      result = LLVMBuildICmp(ctx->builder, LLVMIntSGT, left, right, "gttmp");
      break;
    case TOKEN_LTE:
      result = LLVMBuildICmp(ctx->builder, LLVMIntSLE, left, right, "ltetmp");
      break;
    case TOKEN_GTE:
      result = LLVMBuildICmp(ctx->builder, LLVMIntSGE, left, right, "gtetmp");
      break;
    default:
      fprintf(stderr, "Unknown binary operator: %d\n", node->binary.operator);
      exit(1);
  }
  return result;
}

LLVMValueRef emit_thir_return(LLVM_Emit_Context *ctx, THIR *node) {
  if (node->return_expression) {
    LLVMValueRef ret_val = emit_thir_node(ctx, node->return_expression);
    LLVMBuildRet(ctx->builder, ret_val);
  } else {
    LLVMBuildRetVoid(ctx->builder);
  }
  return nullptr;
}

LLVMValueRef emit_thir_block(LLVM_Emit_Context *ctx, THIR *node) {
  for (size_t i = 0; i < node->statements.length; ++i) {
    emit_thir_node(ctx, node->statements.nodes[i]);
  }
  return nullptr;
}

LLVMValueRef emit_thir_node(LLVM_Emit_Context *ctx, THIR *node) {
  switch (node->kind) {
    case THIR_PROGRAM:
      return emit_thir_program(ctx, node);
    case THIR_BLOCK:
      return emit_thir_block(ctx, node);
    case THIR_TYPE_DECLARATION:
      return emit_thir_type_declaration(ctx, node);
    case THIR_FUNCTION:
      return emit_thir_function(ctx, node);
    case THIR_VARIABLE_DECLARATION:
      return emit_thir_variable_declaration(ctx, node);
    case THIR_IDENTIFIER:
      return emit_thir_identifier(ctx, node);
    case THIR_NUMBER:
      return emit_thir_number(ctx, node);
    case THIR_STRING:
      return emit_thir_string(ctx, node);
    case THIR_MEMBER_ACCESS:
      return emit_thir_member_access(ctx, node);
    case THIR_CALL:
      return emit_thir_call(ctx, node);
    case THIR_BINARY_EXPRESSION:
      return emit_thir_binary_expression(ctx, node);
    case THIR_RETURN:
      return emit_thir_return(ctx, node);
    default:
      fprintf(stderr, "Unknown THIR node kind: %d\n", node->kind);
      exit(1);
  }
}