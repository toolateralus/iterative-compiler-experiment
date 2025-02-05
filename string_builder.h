#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr size_t MAX_BUILDER_NODE_LENGTH = 1024 * 8;

typedef struct String_Builder_Node {
  char *data;
  size_t length;
  struct String_Builder_Node *next;
} String_Builder_Node;

typedef struct {
  String_Builder_Node *root_node;
  String_Builder_Node *current;
} String_Builder;

static void string_builder_init(String_Builder *builder) {
  builder->root_node = malloc(sizeof(String_Builder_Node)),
  builder->root_node->data = malloc(MAX_BUILDER_NODE_LENGTH);
  builder->root_node->length = 0;
  builder->root_node->next = NULL;
  builder->current = builder->root_node;
}

static char string_builder_printf_buffer[MAX_BUILDER_NODE_LENGTH] = {0};

static inline void string_builder_appendf(String_Builder *builder, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int length = vsnprintf(string_builder_printf_buffer, MAX_BUILDER_NODE_LENGTH, format, args);
  va_end(args);

  const char *ptr = string_builder_printf_buffer;
  while (length > 0) {
    if (builder->current->length == MAX_BUILDER_NODE_LENGTH) {
      String_Builder_Node *new_node = malloc(sizeof(String_Builder_Node));
      new_node->data = malloc(MAX_BUILDER_NODE_LENGTH);
      new_node->length = 0;
      new_node->next = NULL;
      builder->current->next = new_node;
      builder->current = new_node;
    }
    size_t space_left = MAX_BUILDER_NODE_LENGTH - builder->current->length;
    size_t copy_length = length < space_left ? length : space_left;
    memcpy(builder->current->data + builder->current->length, ptr, copy_length);
    builder->current->length += copy_length;
    ptr += copy_length;
    length -= copy_length;
  }
}

static inline char *string_builder_get_string(String_Builder *builder) {
  size_t total_length = 0;
  String_Builder_Node *node = builder->root_node;
  while (node) {
    total_length += node->length;
    node = node->next;
  }
  char *result = (char *)malloc(total_length + 1);
  char *current_position = result;
  node = builder->root_node;
  while (node) {
    memcpy(current_position, node->data, node->length);
    current_position += node->length;
    node = node->next;
  }
  result[total_length] = '\0';
  return result;
}

static inline void string_builder_free(String_Builder *builder) {
  String_Builder_Node *node = builder->root_node;
  while (node) {
    String_Builder_Node *next = node->next;
    free(node->data);
    free(node);
    node = next;
  }
}

#endif // STRING_BUILDER_H