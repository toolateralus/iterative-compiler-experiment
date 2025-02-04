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
  String string = {
    .data = malloc(sizeof(char) * length),
    .length = length
  };
  memcpy(string.data, data, length * sizeof(char));
  return string;
}

static bool String_equals(String a, const char *b) {
  auto length = strlen(b);
  if (a.length != length) return false;
  auto longest = a.length > length ? a.length : length;
  return strncmp(a.data, b, longest) == 0;
}

static bool Strings_compare(String a, String b) {
  if (a.length != b.length) return false;

  auto longest = a.length > b.length ? a.length : b.length;
  return strncmp(a.data, b.data, longest) == 0;
}

#endif