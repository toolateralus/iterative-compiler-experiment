#ifndef LEX_H
#define LEX_H
#include "core.h"
#include <ctype.h>
#include <stdio.h>

typedef enum {
  TOKEN_EOF_OR_INVALID,
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,

  TOKEN_FN_KEYWORD,
  TOKEN_TYPE_KEYWORD,

  TOKEN_AT,
  TOKEN_DOT,
  TOKEN_COMMA,
  TOKEN_ASSIGN,
  TOKEN_SEMICOLON,

  TOKEN_OPEN_PAREN,
  TOKEN_CLOSE_PAREN,

  TOKEN_OPEN_CURLY,
  TOKEN_CLOSE_CURLY,

  TOKEN_ADD,
  TOKEN_SUB,
  TOKEN_DIV,
  TOKEN_MUL,
  TOKEN_MOD,

  TOKEN_AND,
  TOKEN_OR,
  TOKEN_XOR,
  TOKEN_SHL,
  TOKEN_SHR,

  TOKEN_EQ,
  TOKEN_NEQ,
  TOKEN_LOGICAL_NOT,
  TOKEN_LOGICAL_OR,

  TOKEN_LT,
  TOKEN_GT,
  TOKEN_GTE,
  TOKEN_LTE,

} Token_Type;

static const char *Token_Type_Name(Token_Type type) {
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
  case TOKEN_AT:
    return "TOKEN_AT";
  case TOKEN_DOT:
    return "TOKEN_DOT";
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

typedef struct Source_Location {
  int line, column;
  const char *file;
} Source_Location;

typedef struct {
  Token_Type type;
  String value;
  Source_Location location;
} Token;

typedef struct {
  char *content;
  size_t length;
  size_t position;
  Token lookahead[8];
  size_t lookahead_length;
  Source_Location location;
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
  state->lookahead_length = 0;
  state->location = (Source_Location){
      .column = 1,
      .line = 1,
      .file = filename,
  };
  memset(state->lookahead, 0, sizeof(state->lookahead));
}

static char lexer_state_peek_char(Lexer_State *state) {
  if (state->position >= state->length) {
    return EOF;
  }
  return state->content[state->position];
}

static char lexer_state_eat_char(Lexer_State *state) {
  if (state->position >= state->length) {
    return EOF;
  }
  char c = state->content[state->position];
  if (c == '\n') {
    state->location.line++;
    state->location.column = 1;
  } else {
    state->location.column++;
  }
  return state->content[state->position++];
}

typedef struct {
  const char *key;
  Token_Type value;
} Operator;

static Operator operator_map[] = {
    {"@", TOKEN_AT},          {"=", TOKEN_ASSIGN},      {",", TOKEN_COMMA},
    {")", TOKEN_CLOSE_PAREN}, {"}", TOKEN_CLOSE_CURLY}, {".", TOKEN_DOT},
    {";", TOKEN_SEMICOLON},   {"(", TOKEN_OPEN_PAREN},  {"{", TOKEN_OPEN_CURLY},
};

typedef struct Keyword {
  const char *key;
  Token_Type value;
} Keyword;

static Keyword keyword_map[] = {
    {"fn", TOKEN_FN_KEYWORD},
    {"type", TOKEN_TYPE_KEYWORD},
};

static Token get_token(Lexer_State *state) {
  Token token;
  token.location = state->location;
  token.type = TOKEN_EOF_OR_INVALID;

  while (1) {
    char c = lexer_state_eat_char(state);
    if (c == EOF) {
      return token;
    }

    // Multi-line comments
    // Delimited by ## and ##
    if (c == '#' && lexer_state_peek_char(state) == '#') {
      while (1) {
        c = lexer_state_eat_char(state);
        if (c == '#' && lexer_state_peek_char(state) == '#') {
          lexer_state_eat_char(state);
          break;
        }
      }
      continue;
    }

    if (c == '#') {
      c = lexer_state_eat_char(state);
      while ((c = lexer_state_eat_char(state)) != '\n')
        ;
      continue;
    }

    if (c == ' ' || c == '\n' || c == '\t') {
      continue;
    }

    if (c == '"') {
      token.type = TOKEN_STRING;
      char *start = &state->content[state->position];
      size_t length = 0;
      while ((c = lexer_state_eat_char(state)) != '"' && c != EOF) {
        length++;
      }
      if (c == EOF) {
        token.type = TOKEN_EOF_OR_INVALID;
      }
      token.value = String_new(start, length);
      return token;
    }

    if (isalpha(c) || c == '_') {
      token.type = TOKEN_IDENTIFIER;
      char *start = &state->content[state->position - 1];
      size_t length = 1;
      while (isalnum(c = lexer_state_eat_char(state)) || c == '_') {
        length++;
      }
      state->position--; // Unread the last character
      token.value = String_new(start, length);
      for (int i = 0; i < sizeof(keyword_map) / sizeof(Keyword); ++i) {
        if (String_equals(token.value, keyword_map[i].key)) {
          token.type = keyword_map[i].value;
          break;
        }
      }
      return token;
    } else if (isdigit(c)) {
      token.type = TOKEN_NUMBER;
      char *start = &state->content[state->position - 1];
      size_t length = 1;
      while (isdigit(c = lexer_state_eat_char(state))) {
        length++;
      }
      state->position--; // Unread the last character
      token.value = String_new(start, length);
      return token;
    } else if (ispunct(c)) {
      char next_char = lexer_state_peek_char(state);
      char punct[3] = {c, next_char, '\0'};
      Token_Type type = TOKEN_EOF_OR_INVALID;
      size_t length = 1;

      if (ispunct(next_char)) {
        for (int i = 0; i < sizeof(operator_map) / sizeof(Operator); ++i) {
          if (strcmp(punct, operator_map[i].key) == 0) {
            type = operator_map[i].value;
            lexer_state_eat_char(state);
            length = 2;
            break;
          }
        }
      }

      if (type == TOKEN_EOF_OR_INVALID) {
        punct[1] = '\0';
        for (int i = 0; i < sizeof(operator_map) / sizeof(Operator); ++i) {
          if (strcmp(punct, operator_map[i].key) == 0) {
            type = operator_map[i].value;
            break;
          }
        }
      }

      if (type == TOKEN_EOF_OR_INVALID) {
        fprintf(stderr, "Unknown operator %s\n", punct);
        exit(1);
      }

      token.type = type;
      token.value =
          String_new(&state->content[state->position - length], length);
      return token;
    } else {
      return token;
    }
  }
}

static inline void lexer_state_populate_lookahead_buffer(Lexer_State *state) {
  while (state->lookahead_length < 8) {
    state->lookahead[state->lookahead_length++] = get_token(state);
  }
}

static inline Token token_peek(Lexer_State *state) {
  lexer_state_populate_lookahead_buffer(state);
  return state->lookahead[0];
}

static inline Token token_eat(Lexer_State *state) {
  lexer_state_populate_lookahead_buffer(state);
  Token token = state->lookahead[0];
  for (int i = 1; i < state->lookahead_length; ++i) {
    state->lookahead[i - 1] = state->lookahead[i];
  }
  state->lookahead_length--;
  return token;
}

static inline Token token_lookahead(Lexer_State *state, int n) {
  lexer_state_populate_lookahead_buffer(state);
  if (n >= state->lookahead_length) {
    panic("lookahead out of bounds");
  }
  return state->lookahead[n];
}

static inline Token token_expect(Lexer_State *state, Token_Type type) {
  Token token = token_peek(state);
  if (token.type != type) {
    fprintf(stderr, "Error: expected %s, but got %s\n", Token_Type_Name(type),
            Token_Type_Name(token.type));
    exit(1);
  }
  return token_eat(state);
}

#endif