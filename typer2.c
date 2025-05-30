#include "typer2.h"
#include "core.h"
#include "parser.h"
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
  if (!node) {
    panic("Null node in 'generate_thir_from_ast'");
  }
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
      THIR *base = generate_thir_from_ast(node->dot.left, thir_symbols);
      THIR *thir = THIR_ALLOC(THIR_MEMBER_ACCESS, node->location);
      thir->member_access.base = base;
      thir->member_access.member = node->dot.member_name;

      Type *base_type = get_type(base->type);

      thir->type = -1;
      for (int i = 0; i < base_type->$struct.members.length; ++i) {
        Type_Member member = V_AT(Type_Member, base_type->$struct.members, i);
        if (Strings_compare(member.name, node->dot.member_name)) {
          thir->type = member.type;
          break;
        }
      }

      if (thir->type == -1) {
        parse_panicf(node->location, "unable to find member '%s' in type '%s'", node->dot.member_name.data,
                     base_type->name.data);
      }

      return thir;
    } break;
    case AST_NODE_FUNCTION_CALL: {
      printf("n thir_syms=%zu\n", thir_symbols->length);
      for (int i = 0; i < thir_symbols->length; ++i) {
        THIRSymbol symbol = V_AT(THIRSymbol, *thir_symbols, i);
        printf("symbol: %s\n", symbol.name.data);
      }
      THIRSymbol *symbol = find_thir_symbol(thir_symbols, node->call.name);

      if (!symbol) {
        parse_panicf(node->location, "use of undeclared function '%s'", node->call.name.data);
      }

      THIR *function = symbol->thir;
      THIR *thir = THIR_ALLOC(THIR_CALL, node->location);
      for (int i = 0; i < node->call.arguments.length; ++i) {
        AST *argument = V_AT(AST *, node->call.arguments, i);
        THIR *thir_arg = generate_thir_from_ast(argument, thir_symbols);
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
      return thir;
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
      bool is_varargs = false;
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
          is_varargs = true;
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
      Type *new_type = create_type(node, node->declaration.name, STRUCT);
      vector_init(&thir->type_declaration.members, sizeof(THIRMember));
      thir->type_declaration.name = node->declaration.name;
      for (int i = 0; i < node->declaration.members.length; ++i) {
        AST_Type_Member member = V_AT(AST_Type_Member, node->declaration.members, i);
        THIRMember thir_member = {.type = find_type(member.type)->id, .name = member.name};

        vector_push(&new_type->$struct.members, &(Type_Member){
                                                    .name = thir_member.name,
                                                    .type = thir_member.type,
                                                });
      }

      vector_push(thir_symbols, &(THIRSymbol){
                                    .thir = thir,
                                    .name = node->declaration.name,
                                });
      thir->type = new_type->id;
      return thir;
    } break;
    case AST_NODE_VARIABLE_DECLARATION: {
      THIR *thir = THIR_ALLOC(THIR_VARIABLE_DECLARATION, node->location);

      size_t expected_type = find_type(node->variable.type)->id;
      thir->variable.name = node->variable.name;

      if (node->variable.value) {
        thir->variable.value = generate_thir_from_ast(node->variable.value, thir_symbols);

        size_t expr_type = thir->variable.value->type;
        if (expected_type != expr_type) {
          parse_panic(node->location, "invalid type in variable declaration");
        }

      } else {
        thir->variable.value = NULL;
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

THIR *generate_thir(DepGraph *graph, DepNodeRegistry *registry, Vector *thir_symbols) {
  THIR *program = THIR_ALLOC(THIR_PROGRAM, (Source_Location){0});
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

  free(in_degree);
  free(queue);

  // Ensure all nodes are processed (handles disconnected components/cycles)
  for (size_t i = 0; i < n; ++i) {
    if (graph->nodes[i]->state != RESOLVED) {
      generate_thir_for_node(graph->nodes[i], thir_symbols, program);
    }
  }

  return program;
}

void pretty_print_thir(THIR *thir, int indent) {
  if (!thir) {
    print_indent(indent);
    printf("<null>\n");
    return;
  }

  print_indent(indent);
  printf("<kind=(%d :: %s), type=(%s)>\n", thir->kind, thir_kind_to_string(thir->kind),
         type_to_string(get_type(thir->type)).data);

  switch (thir->kind) {
    case THIR_PROGRAM:
    case THIR_BLOCK:
      for (size_t i = 0; i < thir->statements.length; ++i) {
        pretty_print_thir(thir->statements.nodes[i], indent + 1);
      }
      break;

    case THIR_BINARY_EXPRESSION:
      pretty_print_thir(thir->binary.left, indent + 1);
      print_indent(indent + 1);
      printf("<operator :: '%s'>\n", Token_Type_Name(thir->binary.operator));
      pretty_print_thir(thir->binary.right, indent + 1);
      break;

    case THIR_CALL:
      print_indent(indent + 1);
      printf("<function :: %s>\n", thir->call.function->function.name.data);
      print_indent(indent + 1);
      printf("<args>\n");
      for (size_t i = 0; i < thir->call.arguments.length; ++i) {
        pretty_print_thir(thir->call.arguments.nodes[i], indent + 2);
      }
      break;

    case THIR_MEMBER_ACCESS:
      print_indent(indent + 1);
      printf("<base>\n");
      pretty_print_thir(thir->member_access.base, indent + 2);
      print_indent(indent + 1);
      printf("<member> ");
      print_string(thir->member_access.member);
      printf("\n");
      break;

    case THIR_IDENTIFIER:
      print_indent(indent + 1);
      printf("Identifier: ");
      print_string(thir->identifier.name);
      printf("\n");
      break;

    case THIR_NUMBER:
      print_indent(indent + 1);
      printf("Number: ");
      print_string(thir->number);
      printf("\n");
      break;

    case THIR_STRING:
      print_indent(indent + 1);
      printf("String: ");
      print_string(thir->string);
      printf("\n");
      break;

    case THIR_RETURN:
      print_indent(indent + 1);
      printf("Return\n");
      pretty_print_thir(thir->return_expression, indent + 2);
      break;

    case THIR_FUNCTION:
      print_indent(indent + 1);
      printf("Function: ");
      print_string(thir->function.name);
      printf("%s%s\n", thir->function.is_extern ? " [extern]" : "", thir->function.is_entry ? " [entry]" : "");
      print_indent(indent + 1);
      printf("<params>\n");
      for (size_t i = 0; i < thir->function.parameters.length; ++i) {
        THIRParameter *param = V_PTR_AT(THIRParameter, thir->function.parameters, i);
        print_indent(indent + 2);
        print_string(param->name);
        printf(" : %zu%s\n", param->type, param->is_vararg ? " (vararg)" : "");
      }
      print_indent(indent + 1);
      printf("<block>\n");
      pretty_print_thir(thir->function.block, indent + 2);
      break;

    case THIR_TYPE_DECLARATION:
      print_indent(indent + 1);
      printf("Type: ");
      print_string(thir->type_declaration.name);
      printf("\n");
      for (size_t i = 0; i < thir->type_declaration.members.length; ++i) {
        THIRMember *member = V_PTR_AT(THIRMember, thir->type_declaration.members, i);
        print_indent(indent + 2);
        print_string(member->name);
        printf(" : %zu\n", member->type);
      }
      break;

    case THIR_VARIABLE_DECLARATION:
      print_indent(indent + 1);
      printf("Variable: ");
      print_string(thir->variable.name);
      printf("\n");
      if (thir->variable.value) {
        print_indent(indent + 2);
        printf("Value:\n");
        pretty_print_thir(thir->variable.value, indent + 3);
      }
      break;

    default:
      print_indent(indent + 1);
      printf("<unknown THIR kind>\n");
      break;
  }
}

const char *thir_kind_to_string(THIRKind type) {
  switch (type) {
    THIRTypeNameCase(THIR_PROGRAM) THIRTypeNameCase(THIR_BLOCK) THIRTypeNameCase(THIR_BINARY_EXPRESSION)
        THIRTypeNameCase(THIR_CALL) THIRTypeNameCase(THIR_MEMBER_ACCESS) THIRTypeNameCase(THIR_IDENTIFIER)
            THIRTypeNameCase(THIR_NUMBER) THIRTypeNameCase(THIR_STRING) THIRTypeNameCase(THIR_RETURN)
                THIRTypeNameCase(THIR_FUNCTION) THIRTypeNameCase(THIR_TYPE_DECLARATION)
                    THIRTypeNameCase(THIR_VARIABLE_DECLARATION) default : return "INVALID_THIR_KIND";
  }
}
