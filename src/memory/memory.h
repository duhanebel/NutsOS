#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void *memset(void *ptr, uint8_t c, size_t size);
int memcmp(void *s1, void *s2, int count);
#endif
