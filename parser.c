#include "parser.h"
#include "lexer.h"

AST *parse_binary_expression(AST_Arena *arena, Lexer_State *state, AST *parent) {
  AST *left = parse_expression(arena, state, parent);
  while (is_binary_operator(token_peek(state).type)) {
    Token operator = token_eat(state);
    AST *right = parse_expression(arena, state, parent);
    AST *binary_node = ast_arena_alloc(state, arena, AST_NODE_BINARY_EXPRESSION, parent);
    binary_node->binary.left = left;
    binary_node->binary.operator = operator.type;
    binary_node->binary.right = right;
    left = binary_node;
  }
  return left;
}

AST *parse_expression(AST_Arena *arena, Lexer_State *state, AST *parent) {
  Token token = token_eat(state);

  switch (token.type) {
  case TOKEN_STRING: {
    AST *node = ast_arena_alloc(state, arena, AST_NODE_STRING, parent);
    node->string = token.value;
    return node;
  }
  case TOKEN_IDENTIFIER: {
    Token next_token = token_peek(state);

    if (next_token.type == TOKEN_DOT) { // Parse dot expressions
      token_eat(state);                 // Consume '.'
      AST *dot_node = ast_arena_alloc(state, arena, AST_NODE_DOT_EXPRESSION, parent);
      dot_node->dot.left = ast_arena_alloc(state, arena, AST_NODE_IDENTIFIER, parent);
      dot_node->dot.left->identifier = token.value;

      dot_node->dot.member_name = token_expect(state, TOKEN_IDENTIFIER).value;
      return dot_node;
    }

    if (next_token.type == TOKEN_OPEN_PAREN) { // Parse function calls
      AST *call_node = ast_arena_alloc(state, arena, AST_NODE_FUNCTION_CALL, parent);
      vector_init(&call_node->call.arguments, sizeof(AST *));
      call_node->call.name = token.value;
      token_eat(state); // Consume '('
      while (token_peek(state).type != TOKEN_CLOSE_PAREN) {
        AST *arg = parse_binary_expression(arena, state, call_node);
        vector_push(&call_node->call.arguments, &arg);
        if (token_peek(state).type != TOKEN_CLOSE_PAREN) {
          token_expect(state, TOKEN_COMMA);
        }
      }
      token_eat(state); // Consume ')'
      return call_node;
    }

    if (next_token.type == TOKEN_IDENTIFIER) { // Parse variable declarations and assignments.
      String type = token.value;
      String name = token_expect(state, TOKEN_IDENTIFIER).value;
      if (token_peek(state).type == TOKEN_ASSIGN) {
        token_eat(state); // Consume '='
        AST *var_decl_node =
            ast_arena_alloc(state, arena, AST_NODE_VARIABLE_DECLARATION, parent);
        var_decl_node->variable.type = type;
        var_decl_node->variable.name = name;
        var_decl_node->variable.default_value =
            parse_binary_expression(arena, state, var_decl_node);
        token_expect(state, TOKEN_SEMICOLON);
        return var_decl_node;
      } else {
        AST *assignment_node =
            ast_arena_alloc(state, arena, AST_NODE_VARIABLE_DECLARATION, parent);
        assignment_node->variable.type = type;
        assignment_node->variable.name = name;
        token_expect(state, TOKEN_SEMICOLON);
        return assignment_node;
      }
    }

    AST *node = ast_arena_alloc(state, arena, AST_NODE_IDENTIFIER, parent);
    node->identifier = token.value;
    return node;
  }
  case TOKEN_NUMBER: {
    AST *node = ast_arena_alloc(state, arena, AST_NODE_NUMBER, parent);
    node->number = token.value;
    return node;
  }
  default: {
    parse_panic(token.location, "Unexpected token in expression");
    return NULL;
  }
  }
}

AST *parse_function_declaration(AST_Arena *arena, Lexer_State *state,
                                AST *parent) {
  token_expect(state, TOKEN_FN_KEYWORD);
  String name = token_expect(state, TOKEN_IDENTIFIER).value;
  AST *node = ast_arena_alloc(state, arena, AST_NODE_FUNCTION_DECLARATION, parent);
  vector_init(&node->function.parameters, sizeof(AST_Parameter));
  node->function.is_entry = false;
  node->function.is_extern = false;
  node->function.name = name;
  token_expect(state, TOKEN_OPEN_PAREN);

  while (token_peek(state).type != TOKEN_CLOSE_PAREN) {
    AST_Parameter param = {0};
    if (token_peek(state).type == TOKEN_DOT &&
        token_lookahead(state, 1).type == TOKEN_DOT &&
        token_lookahead(state, 2).type == TOKEN_DOT) {
      token_eat(state);
      token_eat(state);
      token_eat(state);
      param.is_varargs = true;
    } else {
      param.type = token_expect(state, TOKEN_IDENTIFIER).value;
      if (token_peek(state).type != TOKEN_COMMA &&
          token_peek(state).type != TOKEN_CLOSE_PAREN) {
        param.name = token_expect(state, TOKEN_IDENTIFIER).value;
      }
    }

    vector_push(&node->function.parameters, &param);

    if (token_peek(state).type != TOKEN_CLOSE_PAREN) {
      token_expect(state, TOKEN_COMMA);
    }
  }
  token_eat(state); // Consume ')'

  if (token_peek(state).type == TOKEN_IDENTIFIER) {
    node->function.return_type = token_eat(state).value;
  } else {
    node->function.return_type = (String) {
      .data = "void",
      .length = 4
    };
  }

  while (token_peek(state).type == TOKEN_AT) {
    token_eat(state);
    auto key = token_expect(state, TOKEN_IDENTIFIER).value;
    if (String_equals(key, "extern")) {
      node->function.is_extern = true;
    } else if (String_equals(key, "entry")) {
      node->function.is_entry = true;
    } else {
      parse_panic(state->location,
                  "unexpected identifier for '@...' @tribute :PPP");
    }
  }

  if (node->function.is_extern) {
    token_expect(state, TOKEN_SEMICOLON);
    return node;
  }
  node->function.block = parse_block(arena, state, node);
  return node;
}

AST *parse_type_declaration(AST_Arena *arena, Lexer_State *state, AST *parent) {
  token_expect(state, TOKEN_TYPE_KEYWORD);
  AST *node = ast_arena_alloc(state, arena, AST_NODE_TYPE_DECLARATION, parent);
  vector_init(&node->declaration.members, sizeof(AST_Type_Member));
  String name = token_expect(state, TOKEN_IDENTIFIER).value;
  token_expect(state, TOKEN_OPEN_PAREN);
  node->declaration.name = name;
  while (token_peek(state).type != TOKEN_CLOSE_PAREN) {
    AST_Type_Member member;
    member.type = token_expect(state, TOKEN_IDENTIFIER).value;
    member.name = token_expect(state, TOKEN_IDENTIFIER).value;

    vector_push(&node->declaration.members, &member);

    if (token_peek(state).type != TOKEN_CLOSE_PAREN) {
      token_expect(state, TOKEN_COMMA);
    }
  }
  token_expect(state, TOKEN_CLOSE_PAREN);
  token_expect(state, TOKEN_SEMICOLON);
  return node;
}

AST *parse_block(AST_Arena *arena, Lexer_State *state, AST *parent) {
  AST *node = ast_arena_alloc(state, arena, AST_NODE_BLOCK, parent);
  token_expect(state, TOKEN_OPEN_CURLY);
  while (token_peek(state).type != TOKEN_CLOSE_CURLY) {
    ast_list_push(&node->statements, parse_next_statement(arena, state, node));
  }
  token_expect(state, TOKEN_CLOSE_CURLY);
  return node;
}

AST *parse_next_statement(AST_Arena *arena, Lexer_State *state, AST *parent) {
  Token token = token_peek(state);

  // Done parsing
  if (token.type == TOKEN_EOF_OR_INVALID)
    return nullptr;

  switch (token.type) {
  case TOKEN_RETURN_KEYWORD: {
    token_eat(state);
    AST *node = ast_arena_alloc(state, arena, AST_NODE_RETURN, parent);
    if (token_peek(state).type != TOKEN_SEMICOLON) {
      node->$return = parse_binary_expression(arena, state, parent);
    }
    token_expect(state, TOKEN_SEMICOLON);
    return node;
  }

  case TOKEN_IDENTIFIER: {
    AST* expr = parse_expression(arena, state, parent);

    if (token_peek(state).type == TOKEN_ASSIGN) {
      token_eat(state);
      AST *assign = ast_arena_alloc(state, arena, AST_NODE_BINARY_EXPRESSION, parent);
      assign->binary.operator = TOKEN_ASSIGN;
      assign->binary.left = expr;
      assign->binary.right = parse_binary_expression(arena, state, parent);
      token_expect(state, TOKEN_SEMICOLON);
      expr = assign;
    } else if (expr->kind == AST_NODE_FUNCTION_CALL) {
      token_expect(state, TOKEN_SEMICOLON);
    }
    return expr;
  }
  case TOKEN_FN_KEYWORD: {
    return parse_function_declaration(arena, state, parent);
  }
  case TOKEN_TYPE_KEYWORD: {
    return parse_type_declaration(arena, state, parent);
  }
  default: {
    parse_panicf(state->location, "Unexpected token: %s\n",
                 Token_Type_Name(token.type));
  }
  }
}

void insert_symbol(AST *scope, String name, AST *node, Type *type) {
  auto symbol = find_symbol(scope, name);
  if (symbol) {
    parse_panicf(node->location, "re-declaration of symbol %s\n", name.data);
  }
  symbol = &scope->symbol_table;
  while (symbol->next) {
    symbol = symbol->next;
  }
  symbol->next = malloc(sizeof(Symbol));
  symbol->next->name = name;
  symbol->next->node = node;
  if (type) {
    symbol->next->type = type->id;
  }
  symbol->next->next = NULL; // Initialize the next pointer
}

Symbol *find_symbol(AST *scope, String name) {
  auto symbol = &scope->symbol_table;
  while (symbol) {
    if (Strings_compare(symbol->name, name)) {
      return symbol;
    }
    symbol = symbol->next;
  }
  if (scope->parent) {
    return find_symbol(scope->parent, name);
  }
  return NULL;
}
