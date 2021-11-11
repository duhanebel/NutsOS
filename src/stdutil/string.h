#ifndef STRING_H
#define STRING_H
#include <stdbool.h>
#include <stddef.h>

size_t strlen(const char *str);
size_t strnlen(const char *str, size_t max);
char *strcpy(char *dest, const char *src);
bool isdigit(char c);
int ctoi(char c);

#endif