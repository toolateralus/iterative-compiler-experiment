#include "parser.h"
#include "lexer.h"

// parse string, identifier, function call, numbers, dot expressions
AST *parse_expression(AST_Arena *arena, Lexer_State *state) {
  Token token = token_eat(state);

  switch (token.type) {
  case TOKEN_STRING: {
    AST *node = ast_arena_alloc(arena, AST_NODE_STRING);
    node->string = token.value;
    return node;
  }
  case TOKEN_IDENTIFIER: {
    Token next_token = token_peek(state);

    if (next_token.type == TOKEN_DOT) { // Parse dot expressions
      token_eat(state);                 // Consume '.'
      AST *dot_node = ast_arena_alloc(arena, AST_NODE_DOT_EXPRESSION);
      dot_node->dot_expression.left = token.value;
      dot_node->dot_expression.right =
          token_expect(state, TOKEN_IDENTIFIER).value;
      if (token_peek(state).type == TOKEN_ASSIGN) { // Parse dot assignment
        token_eat(state); // Consume '='
        dot_node->dot_expression.assignment_value = parse_expression(arena, state);
        token_expect(state, TOKEN_SEMICOLON);
      }
      return dot_node;
    }

    if (next_token.type == TOKEN_OPEN_PAREN) { // Parse function calls
      AST *call_node = ast_arena_alloc(arena, AST_NODE_FUNCTION_CALL);
      call_node->function_call.name = token.value;
      token_eat(state); // Consume '('
      while (token_peek(state).type != TOKEN_CLOSE_PAREN) {
        call_node->function_call
            .arguments[call_node->function_call.arguments_length++] =
            parse_expression(arena, state);
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
        AST *var_decl_node =
            ast_arena_alloc(arena, AST_NODE_VARIABLE_DECLARATION);
        var_decl_node->variable_declaration.type = type;
        var_decl_node->variable_declaration.name = name;
        var_decl_node->variable_declaration.default_value =
            parse_expression(arena, state);
        token_expect(state, TOKEN_SEMICOLON);
        return var_decl_node;
      } else {
        AST *assignment_node =
            ast_arena_alloc(arena, AST_NODE_VARIABLE_DECLARATION);
        assignment_node->variable_declaration.type = type;
        assignment_node->variable_declaration.name = name;
        token_expect(state, TOKEN_SEMICOLON);
        return assignment_node;
      }
    }

    AST *node = ast_arena_alloc(arena, AST_NODE_IDENTIFIER);
    node->identifier = token.value;
    return node;
  }
  case TOKEN_NUMBER: {
    AST *node = ast_arena_alloc(arena, AST_NODE_NUMBER);
    node->number = token.value;
    return node;
  }
  default: {
    panic("Unexpected token in expression");
    return NULL;
  }
  }
}

AST *parse_function_declaration(AST_Arena *arena, Lexer_State *state) {
  token_expect(state, TOKEN_FN_KEYWORD);
  String name = token_expect(state, TOKEN_IDENTIFIER).value;
  AST *node = ast_arena_alloc(arena, AST_NODE_FUNCTION_DECLARATION);
  node->function_declaration.is_entry = false;
  node->function_declaration.is_extern = false;
  node->function_declaration.name = name;
  token_expect(state, TOKEN_OPEN_PAREN);
  while (token_peek(state).type != TOKEN_CLOSE_PAREN) {
    Parameter param;
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

    node->function_declaration
        .parameters[node->function_declaration.parameters_length++] = param;
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
      panic("unexpected identifier for '@...' AT-tribute :PPP");
    }
  }

  if (node->function_declaration.is_extern) {
    token_expect(state, TOKEN_SEMICOLON);
    return node;
  }
  node->function_declaration.body = parse_block(arena, state);
  return node;
}

AST *parse_type_declaration(AST_Arena *arena, Lexer_State *state) {
  token_expect(state, TOKEN_TYPE_KEYWORD);
  AST *node = ast_arena_alloc(arena, AST_NODE_TYPE_DECLARATION);
  String name = token_expect(state, TOKEN_IDENTIFIER).value;
  token_expect(state, TOKEN_OPEN_PAREN);
  token_expect(state, TOKEN_CLOSE_PAREN);

  token_expect(state, TOKEN_OPEN_CURLY);
  node->type_declaration.name = name;
  while (token_peek(state).type != TOKEN_CLOSE_CURLY) {
    AST_Type_Member member;
    member.type = token_expect(state, TOKEN_IDENTIFIER).value;
    member.name = token_expect(state, TOKEN_IDENTIFIER).value;
    node->type_declaration.members[node->type_declaration.members_length++] =
        member;
    if (token_peek(state).type != TOKEN_CLOSE_CURLY) {
      token_expect(state, TOKEN_COMMA);
    }
  }
  token_expect(state, TOKEN_CLOSE_CURLY);
  return node;
}

AST *parse_block(AST_Arena *arena, Lexer_State *state) {
  AST *node = ast_arena_alloc(arena, AST_NODE_BLOCK);
  token_expect(state, TOKEN_OPEN_CURLY);
  while (token_peek(state).type != TOKEN_CLOSE_CURLY) {
    ast_list_push(&node->block, parse_next_statement(arena, state));
  }
  token_expect(state, TOKEN_CLOSE_CURLY);
  return node;
}

AST *parse_next_statement(AST_Arena *arena, Lexer_State *state) {
  Token token = token_peek(state);

  // Done parsing
  if (token.type == TOKEN_EOF_OR_INVALID)
    return nullptr;

  switch (token.type) {
  case TOKEN_IDENTIFIER: {
    return parse_expression(arena, state);
  }
  case TOKEN_FN_KEYWORD: {
    return parse_function_declaration(arena, state);
  }
  case TOKEN_TYPE_KEYWORD: {
    return parse_type_declaration(arena, state);
    return NULL;
  }
  default: {
    fprintf(stderr, "Unexpected token: %s\n", Token_Type_Name(token.type));
    exit(1);
  }
  }
}