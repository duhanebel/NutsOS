#include "memory.h"

void *memset(void *ptr, uint8_t c, size_t size)
{
  uint8_t *c_ptr = (uint8_t *)ptr;
  for (int i = 0; i < size; i++) {
    c_ptr[i] = c;
  }
  return ptr;
}

int memcmp(const void *s1, const void *s2, int count)
{
  unsigned char *t1 = (unsigned char *)s1;
  unsigned char *t2 = (unsigned char *)s2;
  while (count-- && *t1 == *t2) {
    t1++;
    t2++;
  }
  if (count == -1) {
    return 0;
  }

  return *t2 - *t1;
}

void memcpy(const void *s1, const void *s2, int count)
{
  unsigned char *t1 = (unsigned char *)s1;
  unsigned char *t2 = (unsigned char *)s2;
  while (count--) {
    *t1++ = *t2++;
  }
}
