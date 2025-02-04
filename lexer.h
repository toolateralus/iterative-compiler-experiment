#ifndef LEX_H
#define LEX_H
#include "core.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TOKEN_EOF_OR_INVALID,
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,

  TOKEN_FN_KEYWORD,
  TOKEN_TYPE_KEYWORD,
  TOKEN_I32_KEYWORD,

  TOKEN_AT,
  TOKEN_DOT,
  TOKEN_COMMA,
  TOKEN_ASSIGN,
  TOKEN_SEMICOLON,

  TOKEN_OPEN_PAREN,
  TOKEN_CLOSE_PAREN,

  TOKEN_OPEN_CURLY,
  TOKEN_CLOSE_CURLY,
} Token_Type;

const char *Token_Type_Name(Token_Type type) {
  switch (type) {
  case TOKEN_EOF_OR_INVALID:
    return "TOKEN_EOF_OR_INVALID";
  case TOKEN_IDENTIFIER:
    return "TOKEN_IDENTIFIER";
  case TOKEN_STRING:
    return "TOKEN_STRING";
  case TOKEN_NUMBER:
    return "TOKEN_NUMBER";
  case TOKEN_FN_KEYWORD:
    return "TOKEN_FN_KEYWORD";
  case TOKEN_TYPE_KEYWORD:
    return "TOKEN_TYPE_KEYWORD";
  case TOKEN_I32_KEYWORD:
    return "TOKEN_I32_KEYWORD";
  case TOKEN_AT:
    return "TOKEN_AT";
  case TOKEN_DOT:
    return "TOKEN_COLON";
  case TOKEN_COMMA:
    return "TOKEN_COMMA";
  case TOKEN_ASSIGN:
    return "TOKEN_ASSIGN";
  case TOKEN_SEMICOLON:
    return "TOKEN_SEMICOLON";
  case TOKEN_OPEN_PAREN:
    return "TOKEN_OPEN_PAREN";
  case TOKEN_CLOSE_PAREN:
    return "TOKEN_CLOSE_PAREN";
  case TOKEN_OPEN_CURLY:
    return "TOKEN_OPEN_CURLY";
  case TOKEN_CLOSE_CURLY:
    return "TOKEN_CLOSE_CURLY";
  default:
    return "UNKNOWN_TOKEN";
  }
}

typedef struct {
  Token_Type type;
  String value;
} Token;

typedef struct {
  char *content;
  size_t length;
  size_t position;
  Token lookahead[8];
  size_t lookahead_length;
} Lexer_State;

static void free_lexer_state(Lexer_State *state) {
  if (state->content) {
    free(state->content);
  }
}

static void lexer_state_read_file(Lexer_State *state, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    panic("Failed to open file");
  }

  fseek(file, 0, SEEK_END);
  state->length = ftell(file);
  fseek(file, 0, SEEK_SET);

  state->content = (char *)malloc(state->length + 1);
  if (!state->content) {
    panic("Failed to allocate memory for file content");
  }

  fread(state->content, 1, state->length, file);
  state->content[state->length] = '\0';
  fclose(file);

  state->position = 0;

  memset(state->lookahead, 0, sizeof(state->lookahead));
}

static char lexer_state_get_character(Lexer_State *state) {
  if (state->position >= state->length) {
    return EOF;
  }
  return state->content[state->position++];
}

constexpr static Token_Type punctuation_map[255] = {
  ['@'] = TOKEN_AT,          ['.'] = TOKEN_DOT,
  ['='] = TOKEN_ASSIGN,      [';'] = TOKEN_SEMICOLON,
  [','] = TOKEN_COMMA,       ['('] = TOKEN_OPEN_PAREN,
  [')'] = TOKEN_CLOSE_PAREN, ['{'] = TOKEN_OPEN_CURLY,
  ['}'] = TOKEN_CLOSE_CURLY,
};

typedef struct Keyword {
  const char *key;
  Token_Type value;
} Keyword;

static Keyword keyword_map[] = {
  {"fn", TOKEN_FN_KEYWORD},
  {"type", TOKEN_TYPE_KEYWORD},
  {"i32", TOKEN_I32_KEYWORD},
};

static Token get_token(Lexer_State *state) {
  Token token;
  token.type = TOKEN_EOF_OR_INVALID;
  token.value.start = nullptr;
  token.value.length = 0;

  while (1) {
    char c = lexer_state_get_character(state);
    if (c == EOF) {
      return token;
    }

    if (c == ' ' || c == '\n' || c == '\t') {
      continue;
    }

    if (c == '"') {
      token.type = TOKEN_STRING;
      token.value.start = &state->content[state->position];
      token.value.length = 0;
      while ((c = lexer_state_get_character(state)) != '"' && c != EOF) {
        token.value.length++;
      }
      if (c == EOF) {
        token.type = TOKEN_EOF_OR_INVALID;
      }
      return token;
    }

    if (isalpha(c) || c == '_') {
      token.type = TOKEN_IDENTIFIER;
      token.value.start = &state->content[state->position - 1];
      token.value.length = 1;
      while (isalnum(c = lexer_state_get_character(state)) || c == '_') {
        token.value.length++;
      }
      state->position--; // Unread the last character
      for (int i = 0; i < sizeof(keyword_map) / sizeof(Keyword); ++i) {
        if (strncmp(keyword_map[i].key, token.value.start, token.value.length) == 0) {
          token.type = keyword_map[i].value;
          break;
        }
      }
      return token;
    } else if (isdigit(c)) {
      token.type = TOKEN_NUMBER;
      token.value.start = &state->content[state->position - 1];
      token.value.length = 1;
      while (isdigit(c = lexer_state_get_character(state))) {
        token.value.length++;
      }
      state->position--; // Unread the last character
      return token;
    } else if (ispunct(c)) {
      token.type = punctuation_map[c];
      token.value.start = &state->content[state->position - 1];
      token.value.length = 1;
      return token;
    } else {
      return token;
    }
  }
}

static void lexer_state_populate_lookahead_buffer(Lexer_State *state) {
  while (state->lookahead_length < 8) {
    state->lookahead[state->lookahead_length++] = get_token(state);
  }
}

static Token token_peek(Lexer_State *state) {
  lexer_state_populate_lookahead_buffer(state);
  return state->lookahead[0];
}

static Token token_eat(Lexer_State *state) {
  lexer_state_populate_lookahead_buffer(state);
  Token token = state->lookahead[0];
  for (int i = 1; i < state->lookahead_length; ++i) {
    state->lookahead[i - 1] = state->lookahead[i];
  }
  state->lookahead_length--;
  return token;
}

static Token token_lookahead(Lexer_State *state, int n) {
  lexer_state_populate_lookahead_buffer(state);
  if (n >= state->lookahead_length) {
    panic("lookahead out of bounds");
  }
  return state->lookahead[n];
}

static Token token_expect(Lexer_State *state, Token_Type type) {
  Token token = token_peek(state);
  assert(token.type == type && "Unexpected token");
  return token_eat(state);
}

#endif