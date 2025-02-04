#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void panic(const char *message) {
  printf("%s\n", message);
  exit(1);
}



typedef struct {
  const char *start;
  int length;
} String;

static bool String_equals(String a, const char *b) {
  auto length = strlen(b);
  auto longest = a.length > length ? a.length : length;
  return strncmp(a.start, b, longest) == 0;
}

static bool Strings_compare(String a, String b) {
  auto longest = a.length > b.length ? a.length : b.length;
  return strncmp(a.start, b.start, longest) == 0;
}

#endif