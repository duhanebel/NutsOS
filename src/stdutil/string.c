#include "string.h"

#define ASCII_TERM  0
#define ASCII_HTAB  9
#define ASCII_NL    10
#define ASCII_VTAB  11
#define ASCII_LF    12
#define ASCII_CR    13
#define ASCII_SPACE 32
#define ASCII_ZERO  48
#define ASCII_NINE  57
#define ASCII_A     65
#define ASCII_Z     90


bool isdigit(char c)
{
  return c >= ASCII_ZERO && c <= ASCII_NINE;
}

bool isspace(char c)
{
  return c == ASCII_SPACE || (c >= ASCII_HTAB && c <= ASCII_CR);
}

size_t strlen(const char *str)
{
  size_t len = 0;
  while (str[len]) {
    len++;
  }

  return len;
}

size_t strnlen(const char *str, size_t max)
{
  size_t len = 0;
  while (str[len] && len < max) {
    len++;
  }

  return len;
}

int ctoi(char c)
{
  return c - ASCII_ZERO;
}

size_t strnlent(const char *str, int max, char term)
{
  size_t len = 0;
  while (str[len++] != term && len < max) {}

  return len;
}

int istrncmp(const char *s1, const char *s2, int n)
{
  while (n-- && *s1 && (tolower(*s1) == tolower(*s2))) {
    s1++;
    s2++;
  }

  // Reached the end and they're the same
  if (n == -1) {
    return 0;
  }

  // important: convert to uchar before comparing
  return ((unsigned char)tolower(*s1) - (unsigned char)tolower(*s2));
}

int strncmp(const char *s1, const char *s2, int n)
{
  while (n-- && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  // Reached the end and they're the same
  if (n == -1) {
    return 0;
  }

  // important: convert to uchar before comparing
  return (*(unsigned char *)s1 - *(unsigned char *)s2);
}

char *strcpy(char *dest, const char *src)
{
  while ((*(dest++) = *(src++)) != ASCII_TERM) {}
  return dest;
}

char tolower(char s)
{
  s = (s >= ASCII_A && s <= ASCII_Z) ? (s + 32) : s;
  return s;
}


char *ltrim(char *s)
{
  while (*s && isspace(*s)) {
    s++;
  }
  return s;
}

char *rtrim(char *s)
{
  char *end = s + strlen(s) - 1;
  while (s <= end && *end && isspace(*end)) {
    end--;
  }
  *++end = ASCII_TERM;

  return s;
}

char *trim(char *s)
{
  return ltrim(rtrim(s));
}