#ifndef CORE_H
#define CORE_H

typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef signed long long s64;
typedef signed int s32;
typedef signed short s16;
typedef signed char s8;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void panic(const char *message) {
  fprintf(stderr, "%s\n", message);
  exit(1);
}

typedef struct {
  char *data;
  int length;
} String;
static String String_new(char *data, int length) {
  String string = {.data = malloc(sizeof(char) * (length + 1)),
                   .length = length};
  memcpy(string.data, data, length * sizeof(char));
  string.data[length] = '\0'; // Null-terminate the string
  return string;
}

static bool String_equals(String a, const char *b) {
  auto length = strlen(b);
  if (a.length != length)
    return false;
  auto longest = a.length > length ? a.length : length;
  return strncmp(a.data, b, longest) == 0;
}

static bool Strings_compare(String a, String b) {
  if (a.length != b.length)
    return false;

  auto longest = a.length > b.length ? a.length : b.length;
  return strncmp(a.data, b.data, longest) == 0;
}

#define PRINT_COLOR "\033[1;33m"
#define RESET_COLOR "\033[0m"
#define RUN_COLOR "\033[1;32m"
#define TIME_COLOR "\033[1;34m"

#define TIME_DIFF(start, end) ((double)(end - start) / CLOCKS_PER_SEC)

#define PRINT_TIME(label, time_sec)                                            \
  do {                                                                         \
    if (time_sec < 1e-6)                                                       \
      printf("%s%s in %s%.2f ns%s\n", PRINT_COLOR, label, TIME_COLOR,          \
             time_sec * 1e9, RESET_COLOR);                                     \
    else if (time_sec < 1e-3)                                                  \
      printf("%s%s in %s%.2f µs%s\n", PRINT_COLOR, label, TIME_COLOR,          \
             time_sec * 1e6, RESET_COLOR);                                     \
    else if (time_sec < 1)                                                     \
      printf("%s%s in %s%.2f ms%s\n", PRINT_COLOR, label, TIME_COLOR,          \
             time_sec * 1e3, RESET_COLOR);                                     \
    else                                                                       \
      printf("%s%s in %s%.2f s%s\n", PRINT_COLOR, label, TIME_COLOR, time_sec, \
             RESET_COLOR);                                                     \
  } while (0)

#define TIME_REGION(label, code)                                               \
  do {                                                                         \
    clock_t start = clock();                                                   \
    code clock_t end = clock();                                                \
    double time_sec = TIME_DIFF(start, end);                                   \
    PRINT_TIME(label, time_sec);                                               \
  } while (0)

typedef struct {
  void *data;
  size_t element_size;
  size_t length;
  size_t capacity;
} Vector;

static void vector_init(Vector *vector, size_t element_size) {
  vector->data = NULL;
  vector->element_size = element_size;
  vector->length = 0;
  vector->capacity = 0;
}

static void vector_free(Vector *vector) {
  if (vector->data) {
    free(vector->data);
  }
}

static void vector_resize(Vector *vector, size_t new_capacity) {
  vector->data = realloc(vector->data, new_capacity * vector->element_size);
  if (vector->element_size == 0) {
    panic("Vector was uninitialized");
  }
  if (!vector->data) {
    panic("Failed to allocate memory for vector");
  }
  vector->capacity = new_capacity;
}

#define V_BACK(type, vector) (*(type*)vector_back(&vector))

#define V_PTR_BACK(type, vector) (type*)vector_back(&vector)

static void *vector_back(Vector *vector) {
  if (vector->length == 0) {
    panic("Vector is empty");
  }
  return (char *)vector->data + (vector->length - 1) * vector->element_size;
}

static void vector_push(Vector *vector, void *element) {
  if (vector->length == vector->capacity) {
    size_t new_capacity = vector->capacity == 0 ? 1 : vector->capacity * 2;
    vector_resize(vector, new_capacity);
  }
  memcpy((char *)vector->data + vector->length * vector->element_size, element,
         vector->element_size);
  vector->length++;
}

#define ForEachPtr(type, var, list, block) \
  {for (int i = 0; i < list.length; ++i) { type *var = V_PTR_AT(type, list, i); block }}

#define ForEach(type, var, list, block) \
  {for (int i = 0; i < list.length; ++i) { type var = V_AT(type, list, i); block }}

#define V_PTR_AT(type, vector, index) \
  ((type*)vector_get(&vector, index))

#define V_AT(type, vector, index) \
  (*(type*)vector_get(&vector, index))

static void *vector_get(Vector *vector, size_t index) {
  if (index >= vector->length) {
    panic("Index out of bounds");
  }
  return (char *)vector->data + index * vector->element_size;
}

typedef enum {
  CM_DEBUG,
  CM_RELEASE,
} Compilation_Mode; 

extern Compilation_Mode COMPILATION_MODE;

#endif