#include "memory.h"

void* memset(void* ptr, uint8_t c, size_t size)
{
    uint8_t * c_ptr = (uint8_t *) ptr;
    for (int i = 0; i < size; i++) {
        c_ptr[i] = c;
    }
    return ptr;
}
