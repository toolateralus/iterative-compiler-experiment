#include "typer2.h"
#include "thir.h"
#include "type.h"

bool dep_node_dependencies_resolved(DepNode *node) {
  for (int i = 0; i > node->length; ++i) {
    DepNode *dep = node->dependencies[i];
    switch (dep->state) {
      case UNRESOLVED:
      case RESOLVING:
        return false;
      case RESOLVED:
        break;
      case ERRORED:
        printf("error %s\n", dep->error);
        break;
    }
    if (!dep_node_dependencies_resolved(dep)) {
      return false;
    }
  }
  return true;
}

THIR *generate_thir_from_ast(AST *node, Vector *thir_symbols) {
  switch (node->kind) {
    case AST_NODE_IDENTIFIER: {
      THIR *thir = THIR_ALLOC(THIR_IDENTIFIER, node->location);
      THIRSymbol *symbol = find_thir_symbol(thir_symbols, node->identifier);
      thir->identifier = (typeof(thir->identifier)){.name = node->identifier, .resolved = symbol->thir};
      thir->type = symbol->thir->type;
      return thir;
    } break;
    case AST_NODE_NUMBER: {
      THIR *thir = THIR_ALLOC(THIR_NUMBER, node->location);
      thir->number = node->number;
      thir->type = I32;
      return thir;
    } break;
    case AST_NODE_STRING: {
      THIR *thir = THIR_ALLOC(THIR_STRING, node->location);
      thir->string = node->string;
      thir->type = STRING;
      return thir;
    } break;
    case AST_NODE_DOT_EXPRESSION: {
      // TODO: type this.
      THIR *base = generate_thir_from_ast(node->dot.left, thir_symbols);
      THIR *thir = THIR_ALLOC(THIR_MEMBER_ACCESS, node->location);
      thir->member_access.base = base;
      thir->member_access.member = node->dot.member_name;
      return thir;
    } break;
    case AST_NODE_FUNCTION_CALL: {
      THIRSymbol *symbol = find_thir_symbol(thir_symbols, node->call.name);
      THIR *function = symbol->thir;
      THIR *thir = THIR_ALLOC(THIR_CALL, node->location);
      for (int i = 0; i < node->call.arguments.length; ++i) {
        AST *argument = V_AT(AST*, node->call.arguments, i);
        THIR *thir_arg= generate_thir_from_ast(argument, thir_symbols);
        thir_list_push(&thir->call.arguments, thir_arg);
      }
      thir->call.function = function;
      Type *fn_type = get_type(symbol->thir->type);
      thir->type = fn_type->$function.$return;

      return thir;
    } break;
    case AST_NODE_BLOCK: {
      THIR *thir = THIR_ALLOC(THIR_BLOCK, node->location);
      thir->type = VOID;
      thir->statements = (THIRList){0};
      for (int i = 0; i < node->statements.length; ++i) {
        thir_list_push(&thir->statements, generate_thir_from_ast(node->statements.data[i], thir_symbols));
      }
      return thir;
    } break;
    case AST_NODE_BINARY_EXPRESSION: {
      THIR *left = generate_thir_from_ast(node->binary.left, thir_symbols);
      THIR *right = generate_thir_from_ast(node->binary.right, thir_symbols);

      THIR *thir = THIR_ALLOC(THIR_BINARY_EXPRESSION, node->location);
      thir->binary.operator= node->binary.operator;
      thir->binary.left = left;
      thir->binary.right = right;
      thir->type = left->type;
    } break;
    case AST_NODE_RETURN: {
      THIR *thir = THIR_ALLOC(THIR_RETURN, node->location);
      if (node->return_expression) {
        thir->return_expression = generate_thir_from_ast(node->return_expression, thir_symbols);
      }
      thir->type = VOID;
      return thir;
    } break;
    case AST_NODE_FUNCTION_DECLARATION: {
      THIR *thir = THIR_ALLOC(THIR_FUNCTION, node->location);
      if (node->function.block) {
        thir->function.block = generate_thir_from_ast(node->function.block, thir_symbols);
      }

      Vector parameter_types;
      bool is_varargs=false;
      vector_init(&parameter_types, sizeof(size_t));
      vector_init(&thir->function.parameters, sizeof(THIRParameter));
      for (int i = 0; i < node->function.parameters.length; ++i) {
        AST_Parameter *parameter = &node->function.parameters.data[i];

        size_t type;
        Type *type_ptr;
        if (!parameter->is_vararg && (type_ptr = find_type(parameter->type))) {
          type = type_ptr->id;
        } else {
          type = VOID;
        }
        vector_push(&parameter_types, &type);

        if (parameter->is_vararg) {
          is_varargs=true;
        }

        THIRParameter thir_parameter = {
            .type = type,
            .name = parameter->name,
            .is_vararg = parameter->is_vararg,
        };
        vector_push(&thir->function.parameters, &thir_parameter);
      }

      Type *return_type = find_type(node->function.return_type);

      bool new;
      thir->type = create_or_find_function_type(node, return_type->id, parameter_types, is_varargs, &new)->id;

      printf("created new function type for %s?=%s\n", node->function.name.data, new ? "yes" : "no");

      thir->function.is_entry = node->function.is_entry;
      thir->function.is_extern = node->function.is_extern;
      thir->function.name = node->function.name;

      vector_push(thir_symbols, &(THIRSymbol){
                                    .thir = thir,
                                    .name = node->function.name,
                                });

      return thir;
    } break;
    case AST_NODE_TYPE_DECLARATION: {
      THIR *thir = THIR_ALLOC(THIR_TYPE_DECLARATION, node->location);
      vector_init(&thir->type_declaration.members, sizeof(THIRMember));
      thir->type_declaration.name = node->declaration.name;
      for (int i = 0; i < node->declaration.members.length; ++i) {
        AST_Type_Member *member = &node->declaration.members.data[i];
        THIRMember thir_member = {.type = find_type(member->type)->id, .name = member->name};
      }

      vector_push(thir_symbols, &(THIRSymbol){
                                    .thir = thir,
                                    .name = node->declaration.name,
                                });
      return thir;
    } break;
    case AST_NODE_VARIABLE_DECLARATION: {
      THIR *thir = THIR_ALLOC(THIR_VARIABLE_DECLARATION, node->location);
      thir->variable.name = node->variable.name;
      thir->variable.value = generate_thir_from_ast(node->variable.value, thir_symbols);
      size_t expected_type = find_type(node->variable.type)->id;
      size_t expr_type = thir->variable.value->type;
      if (expected_type != expr_type) {
        parse_panic(node->location, "invalid type in variable declaration");
      }
      thir->type = expected_type;
      vector_push(thir_symbols, &(THIRSymbol){
                                    .name = node->variable.name,
                                    .thir = thir,
                                });
      return thir;
    } break;
    case AST_NODE_PROGRAM:
      return nullptr;
  }
  return nullptr;
}

void generate_thir_for_node(DepNode *node, Vector *thir_symbols, THIR *program) {
  if (node->state == RESOLVED) return;

  if (node->state == RESOLVING) {
    parse_panic(node->ast_node->location, "cyclic dependency detected");
    return;
  }

  node->state = RESOLVING;

  for (size_t i = 0; i < node->length; ++i) {
    generate_thir_for_node(node->dependencies[i], thir_symbols, program);
  }

  THIR *thir = generate_thir_from_ast(node->ast_node, thir_symbols);
  printf("Adding %p :: %s to program\n", thir, thir_kind_to_string(thir->kind));
  thir_list_push(&program->statements, thir);

  node->state = RESOLVED;
}

/* THIR *generate_thir(DepGraph *graph, DepNodeRegistry *registry, Vector *thir_symbols) {
  THIR *program = THIR_ALLOC(THIR_PROGRAM, (Source_Location){0});
  vector_init(&program->statements, sizeof(THIR *));

  size_t n = graph->length;
  size_t *in_degree = calloc(n, sizeof(size_t));

  size_t processed = 0;
  size_t qlen = 0;
  DepNode **queue = malloc(n * sizeof(DepNode *));

  // Count in-degrees (num deps per branch)
  for (size_t i = 0; i < n; ++i) {
    in_degree[i] = graph->nodes[i]->length;
  }

  // Queue initial nodes, those with 0 deps.
  for (size_t i = 0; i < n; ++i) {
    if (in_degree[i] == 0) {
      queue[qlen++] = graph->nodes[i];
    }
  }

  while (processed < qlen) {
    DepNode *node = queue[processed++];
    generate_thir_for_node(node, thir_symbols, program);

    // For each node that depends on this node, decrement in-degree
    for (size_t i = 0; i < n; ++i) {
      DepNode *other = graph->nodes[i];
      for (size_t j = 0; j < other->length; ++j) {
        if (other->dependencies[j] == node) {
          if (--in_degree[i] == 0) {
            queue[qlen++] = other;
          }
        }
      }
    }
  }

  if (qlen != n) {
    parse_panic((Source_Location){0}, "Dependency cycle detected in graph");
  }

  free(in_degree);
  free(queue);
  return program;
} */
THIR *generate_thir(DepGraph *graph, DepNodeRegistry *registry, Vector *thir_symbols) {
  THIR *program = THIR_ALLOC(THIR_PROGRAM, (Source_Location){0});
  for (int i = 0; i < graph->length; ++i) {
    DepNode *node = graph->nodes[i];
    generate_thir_for_node(node, thir_symbols, program);
  }

  return program;
}