#ifndef BACKEND_H
#define BACKEND_H

#include "parser.h"
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


LLVMValueRef emit_forward_declaration(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_program(LLVM_Emit_Context *ctx, AST *program);
LLVMValueRef emit_identifier(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_number(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_string(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_function_declaration(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_type_declaration(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_variable_declaration(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_dot_expression(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_function_call(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_block(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_binary_expression(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_return_statement(LLVM_Emit_Context *ctx, AST *node);
LLVMValueRef emit_node(LLVM_Emit_Context *ctx, AST *node);
#endif

