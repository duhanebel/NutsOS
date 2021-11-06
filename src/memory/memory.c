#include "memory.h"

void *memset(void *ptr, uint8_t c, size_t size) {
  uint8_t *c_ptr = (uint8_t *)ptr;
  for (int i = 0; i < size; i++) {
    c_ptr[i] = c;
  }
  return ptr;
}

int memcmp(void *s1, void *s2, int count) {
  while (count-- > 0) {
    int diff = *((char *)s1++) - *((char *)s2++);
    if (diff != 0) {
      return diff > 0 ? 1 : -1;
    }
  }

  return 0;
}
