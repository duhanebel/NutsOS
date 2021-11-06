#include "string.h"

#define ASCII_ZERO 48
#define ASCII_NINE 57

bool isdigit(char c) { return c >= ASCII_ZERO && c <= ASCII_NINE; }

size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len++]) {
  }

  return len;
}

size_t strnlen(const char *str, size_t max) {
  size_t len = 0;
  while (str[len++] && len < max) {
  }

  return len;
}

int ctoi(char c) { return c - ASCII_ZERO; }