#ifndef HEAP_H
#define HEAP_H
#include "config.h"
#include <stddef.h>
#include <stdint.h>

// The heap is a contiguous chunk of memory starting at heap.saddr and continuing
// for heap.table->total * 4096 bytes.
// To keep track of which piece of real memory is in use, we represent each 4096-byte
// chunk as a 1byte entry in the heap_table's entries.
// Each of these entries is a bit-flag in which:
// Bit | Description
// ----+---------------------------------------------------------------
//  0  | Equals 1 if the block is in use, 0 otherwise
// 1-5 | Unused
//  6  | Equals 1 if this is the first block of an allocated chunk
//  7  | Equals 1 if this is not the last block of an allocated chunk

#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00

#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST 0b01000000

typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

struct heap_table {
  HEAP_BLOCK_TABLE_ENTRY *entries;
  size_t total;
};

struct heap {
  struct heap_table *table;

  // Start address of the heap data pool
  void *saddr;
};

// Allocate a heap structure
int heap_create(struct heap *heap, void *ptr, void *end, struct heap_table *table);

// Allocate memory on the heap
void *heap_malloc(struct heap *heap, size_t size);

// Free a memory pointer in the heap
void heap_free(struct heap *heap, void *ptr);
#endif