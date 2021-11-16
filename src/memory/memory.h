#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

void *memset(void *ptr, uint8_t c, size_t size);
int memcmp(const void *s1, const void *s2, int count);
void memcpy(const void *s1, const void *s2, int count);
#endif
