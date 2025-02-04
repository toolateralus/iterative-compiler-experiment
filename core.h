#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <stdlib.h>

static void panic(const char *message) {
  printf("%s\n", message);
  exit(1);
}

typedef struct {
  const char *start;
  int length;
} String;
#endif