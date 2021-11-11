#include "string.h"

#define ASCII_ZERO 48
#define ASCII_NINE 57

bool isdigit(char c)
{
  return c >= ASCII_ZERO && c <= ASCII_NINE;
}

size_t strlen(const char *str)
{
  size_t len = 0;
  while (str[len++]) {}

  return len;
}

size_t strnlen(const char *str, size_t max)
{
  size_t len = 0;
  while (str[len++] && len < max) {}

  return len;
}

int ctoi(char c)
{
  return c - ASCII_ZERO;
}

char *strcpy(char *dest, const char *src)
{
  char *target = dest;
  while ((*(target++) = *(src++)) != 0) {}

  *target = 0x00; // nil terminated

  return dest;
}