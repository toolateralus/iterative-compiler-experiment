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
  String string = {.data = malloc(sizeof(char) * (length + 1)), .length = length};
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

  
#endif