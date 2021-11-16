#ifndef STRING_H
#define STRING_H
#include <stdbool.h>
#include <stddef.h>

size_t strlen(const char *str);
size_t strnlen(const char *str, size_t max);
size_t strnlent(const char *str, int max, char term);
char *strcpy(char *dest, const char *src);
int strncmp(const char *s1, const char *s2, int n);
int istrncmp(const char *s1, const char *s2, int n);

bool isspace(char c);
bool isdigit(char c);

int ctoi(char c);
char tolower(char s);

char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);
#endif