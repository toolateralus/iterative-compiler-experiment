#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr size_t MAX_BUILDER_NODE_LENGTH = 8096;

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
  builder->root_node = (String_Builder_Node *)malloc(sizeof(String_Builder_Node));
  if (!builder->root_node) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }
  builder->root_node->data = (char *)malloc(MAX_BUILDER_NODE_LENGTH);
  if (!builder->root_node->data) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }
  builder->root_node->length = 0;
  builder->root_node->next = NULL;
  builder->current = builder->root_node;
}

static void string_builder_appendf(String_Builder *builder, const char *format, ...) {
  va_list args;
  va_start(args, format);

  char buffer[MAX_BUILDER_NODE_LENGTH];
  int length = vsnprintf(buffer, MAX_BUILDER_NODE_LENGTH, format, args);
  va_end(args);

  if (length < 0) {
    fprintf(stderr, "Formatting error\n");
    return; // Handle error
  }

  const char *ptr = buffer;
  while (length > 0) {
    if (builder->current->length == MAX_BUILDER_NODE_LENGTH) {
      String_Builder_Node *new_node = (String_Builder_Node *)malloc(sizeof(String_Builder_Node));
      if (!new_node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
      }
      new_node->data = (char *)malloc(MAX_BUILDER_NODE_LENGTH);
      if (!new_node->data) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
      }
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

static char *string_builder_get_string(String_Builder *builder) {
  size_t total_length = 0;
  String_Builder_Node *node = builder->root_node;

  while (node) {
    total_length += node->length;
    node = node->next;
  }

  char *result = (char *)malloc(total_length + 1);
  if (!result) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

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

static void string_builder_free(String_Builder *builder) {
  String_Builder_Node *node = builder->root_node;
  while (node) {
    String_Builder_Node *next = node->next;
    free(node->data);
    free(node);
    node = next;
  }
}

#endif // STRING_BUILDER_H