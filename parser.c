#include "parser.h"
#include "lexer.h"

// parse string, identifier, function call, numbers, dot expressions
AST *parse_expression(AST_Arena *arena, Lexer_State *state, AST *parent) {
  Token token = token_eat(state);

  switch (token.type) {
  case TOKEN_STRING: {
    AST *node = ast_arena_alloc(state, arena, AST_NODE_STRING);
    node->string = token.value;
    node->parent = parent;
    return node;
  }
  case TOKEN_IDENTIFIER: {
    Token next_token = token_peek(state);

    if (next_token.type == TOKEN_DOT) { // Parse dot expressions
      token_eat(state);                 // Consume '.'
      AST *dot_node = ast_arena_alloc(state, arena, AST_NODE_DOT_EXPRESSION);
      dot_node->dot_expression.left = token.value;
      dot_node->dot_expression.right =
          token_expect(state, TOKEN_IDENTIFIER).value;
      dot_node->parent = parent;
      if (token_peek(state).type == TOKEN_ASSIGN) { // Parse dot assignment
        token_eat(state);                           // Consume '='
        dot_node->dot_expression.assignment_value =
            parse_expression(arena, state, dot_node);
        token_expect(state, TOKEN_SEMICOLON);
      }
      return dot_node;
    }

    if (next_token.type == TOKEN_OPEN_PAREN) { // Parse function calls
      AST *call_node = ast_arena_alloc(state, arena, AST_NODE_FUNCTION_CALL);
      vector_init(&call_node->function_call.arguments, sizeof(AST*));
      call_node->function_call.name = token.value;
      call_node->parent = parent;
      token_eat(state); // Consume '('
      while (token_peek(state).type != TOKEN_CLOSE_PAREN) {
        AST *arg = parse_expression(arena, state, call_node);
        vector_push(&call_node->function_call.arguments, &arg);
        if (token_peek(state).type != TOKEN_CLOSE_PAREN) {
          token_expect(state, TOKEN_COMMA);
        }
      }
      token_eat(state); // Consume ')'
      token_expect(state, TOKEN_SEMICOLON);
      return call_node;
    }

    if (next_token.type ==
        TOKEN_IDENTIFIER) { // Parse variable declarations and assignments.
      String type = token.value;
      String name = token_expect(state, TOKEN_IDENTIFIER).value;
      if (token_peek(state).type == TOKEN_ASSIGN) {
        token_eat(state); // Consume '='
        AST *var_decl_node = ast_arena_alloc(state, arena, AST_NODE_VARIABLE_DECLARATION);
        var_decl_node->variable_declaration.type = type;
        var_decl_node->variable_declaration.name = name;
        var_decl_node->variable_declaration.default_value = parse_expression(arena, state, var_decl_node);
        var_decl_node->parent = parent;
        token_expect(state, TOKEN_SEMICOLON);
        return var_decl_node;
      } else {
        AST *assignment_node = ast_arena_alloc(state, arena, AST_NODE_VARIABLE_DECLARATION);
        assignment_node->variable_declaration.type = type;
        assignment_node->variable_declaration.name = name;
        assignment_node->parent = parent;
        token_expect(state, TOKEN_SEMICOLON);
        return assignment_node;
      }
    }

    AST *node = ast_arena_alloc(state, arena, AST_NODE_IDENTIFIER);
    node->identifier = token.value;
    node->parent = parent;
    return node;
  }
  case TOKEN_NUMBER: {
    AST *node = ast_arena_alloc(state, arena, AST_NODE_NUMBER);
    node->number = token.value;
    node->parent = parent;
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
  AST *node = ast_arena_alloc(state, arena, AST_NODE_FUNCTION_DECLARATION);
  vector_init(&node->function_declaration.parameters, sizeof(AST_Parameter));
  node->function_declaration.is_entry = false;
  node->function_declaration.is_extern = false;
  node->function_declaration.name = name;
  node->parent = parent;
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

    vector_push(&node->function_declaration.parameters, &param);

    if (token_peek(state).type != TOKEN_CLOSE_PAREN) {
      token_expect(state, TOKEN_COMMA);
    }
  }
  token_eat(state); // Consume ')'

  while (token_peek(state).type == TOKEN_AT) {
    token_eat(state);
    auto key = token_expect(state, TOKEN_IDENTIFIER).value;
    if (String_equals(key, "extern")) {
      node->function_declaration.is_extern = true;
    } else if (String_equals(key, "entry")) {
      node->function_declaration.is_entry = true;
    } else {
      parse_panic(state->location, "unexpected identifier for '@...' @tribute :PPP");
    }
  }

  if (node->function_declaration.is_extern) {
    token_expect(state, TOKEN_SEMICOLON);
    return node;
  }
  node->function_declaration.block = parse_block(arena, state, node);
  return node;
}

AST *parse_type_declaration(AST_Arena *arena, Lexer_State *state, AST *parent) {
  token_expect(state, TOKEN_TYPE_KEYWORD);
  AST *node = ast_arena_alloc(state, arena, AST_NODE_TYPE_DECLARATION);
  vector_init(&node->type_declaration.members, sizeof(AST_Type_Member));
  String name = token_expect(state, TOKEN_IDENTIFIER).value;
  node->parent = parent;
  token_expect(state, TOKEN_OPEN_PAREN);
  node->type_declaration.name = name;
  while (token_peek(state).type != TOKEN_CLOSE_PAREN) {
    AST_Type_Member member;
    member.type = token_expect(state, TOKEN_IDENTIFIER).value;
    member.name = token_expect(state, TOKEN_IDENTIFIER).value;

    vector_push(&node->type_declaration.members, &member);

    if (token_peek(state).type != TOKEN_CLOSE_PAREN) {
      token_expect(state, TOKEN_COMMA);
    }
  }
  token_expect(state, TOKEN_CLOSE_PAREN);
  token_expect(state, TOKEN_SEMICOLON);
  return node;
}

AST *parse_block(AST_Arena *arena, Lexer_State *state, AST *parent) {
  AST *node = ast_arena_alloc(state, arena, AST_NODE_BLOCK);
  node->parent = parent;
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
  case TOKEN_IDENTIFIER: {
    return parse_expression(arena, state, parent);
  }
  case TOKEN_FN_KEYWORD: {
    return parse_function_declaration(arena, state, parent);
  }
  case TOKEN_TYPE_KEYWORD: {
    return parse_type_declaration(arena, state, parent);
  }
  default: {
    parse_panicf(state->location, "Unexpected token: %s\n", Token_Type_Name(token.type));
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
  symbol->next->type = type;
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