#ifndef BACKEND_H
#define BACKEND_H

#include "thir.h"
#include "llvm-c/Types.h"
#include <llvm-c/TargetMachine.h>

typedef struct LLVM_Emit_Context {
  LLVMBuilderRef builder;
  LLVMModuleRef module;
  LLVMContextRef context;
  LLVMDIBuilderRef di_builder;
  LLVMMetadataRef di_file;
  LLVMTargetRef target;
  LLVMTargetDataRef target_data;
  bool dont_load;
  LLVMMetadataRef scope;
} LLVM_Emit_Context;

// THIR-based LLVM emission API
LLVMValueRef emit_thir_node(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_program(LLVM_Emit_Context *ctx, THIR *program);
LLVMValueRef emit_thir_function_forward_declaration(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_function(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_type_declaration(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_variable_declaration(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_identifier(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_number(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_string(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_member_access(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_call(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_binary_expression(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_return(LLVM_Emit_Context *ctx, THIR *node);
LLVMValueRef emit_thir_block(LLVM_Emit_Context *ctx, THIR *node);

#endif