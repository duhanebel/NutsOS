#include "heap.h"
#include "error.h"
#include "kernel.h"
#include "memory/memory.h"
#include <stdbool.h>

static int heap_validate_table(void *ptr, void *end, struct heap_table *table) {
  size_t table_size = (size_t)(end - ptr);
  size_t total_blocks = table_size / NUTSOS_HEAP_BLOCK_SIZE;
  if (table->total != total_blocks) {
    return -EINVARG;
  }
  return 0;
}

// To make our life easier with paging, we only allocate 4kb-aligned chunks of memory
static bool heap_validate_alignment(void *ptr) { return ((unsigned int)ptr % NUTSOS_HEAP_BLOCK_SIZE) == 0; }

int heap_create(struct heap *heap, void *ptr, void *end, struct heap_table *table) {
  int res = 0;

  // Make sure both beginning and end are 4k-aligned
  if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end)) {
    return -EINVARG;
  }

  memset(heap, 0, sizeof(struct heap));
  heap->saddr = ptr;
  heap->table = table;

  res = heap_validate_table(ptr, end, table);
  if (res < 0) {
    return res;
  }

  size_t table_size = sizeof(heap_block_table_entry_t) * table->total;
  memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

  return 0;
}

// Takes a size and return the closest larger size that is aligned to 4k blocks
static uint32_t heap_align_value_to_upper(uint32_t val) {
  if ((val % NUTSOS_HEAP_BLOCK_SIZE) == 0) {
    return val;
  }

  val = (val - (val % NUTSOS_HEAP_BLOCK_SIZE));
  val += NUTSOS_HEAP_BLOCK_SIZE;
  return val;
}

static int heap_get_entry_type(heap_block_table_entry_t entry) { return entry & HEAP_BLOCK_MASK_TYPE; }

int heap_get_next_free_block(struct heap *heap, uint32_t total_blocks) {
  struct heap_table *table = heap->table;
  int bc = 0;
  int bs = -1;

  for (size_t i = 0; i < table->total; i++) {
    // Not free, let's skip the entry, reset the count and move on to the next one
    if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE) {
      bc = 0;
      bs = -1;
      continue;
    }

    // If this is the first block
    if (bs == -1) {
      bs = i;
    }
    bc++;
    // We found space large enough, time to ret
    if (bc == total_blocks) {
      break;
    }
  }

  // Could not find a valid chunk of memory to allocate
  if (bs == -1) {
    return -ENOMEM;
  }

  return bs;
}

void *heap_block_to_address(struct heap *heap, int block) { return heap->saddr + (block * NUTSOS_HEAP_BLOCK_SIZE); }
int heap_address_to_block(struct heap *heap, void *address) { return ((int)(address - heap->saddr)) / NUTSOS_HEAP_BLOCK_SIZE; }

void heap_mark_blocks_taken(struct heap *heap, int start_block, int total_blocks) {
  int end_block = (start_block + total_blocks) - 1;

  heap_block_table_entry_t entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
  if (total_blocks > 1) {
    entry |= HEAP_BLOCK_HAS_NEXT;
  }

  for (int i = start_block; i <= end_block; i++) {
    heap->table->entries[i] = entry;
    entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
    if (i != end_block - 1) {
      entry |= HEAP_BLOCK_HAS_NEXT;
    }
  }
}

void *heap_malloc_blocks(struct heap *heap, uint32_t total_blocks) {
  void *address = 0;

  int start_block = heap_get_next_free_block(heap, total_blocks);

  // Couldn't find any free space, return zero
  if (start_block < 0) {
    return 0;
  }

  address = heap_block_to_address(heap, start_block);

  // Mark the blocks as taken
  heap_mark_blocks_taken(heap, start_block, total_blocks);

  return address;
}

void heap_mark_blocks_free(struct heap *heap, int starting_block) {
  struct heap_table *table = heap->table;
  for (int i = starting_block; i < (int)table->total; i++) {
    heap_block_table_entry_t entry = table->entries[i];
    table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
    if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
      break;
    }
  }
}

void *heap_malloc(struct heap *heap, size_t size) {
  size_t aligned_size = heap_align_value_to_upper(size);
  uint32_t total_blocks = aligned_size / NUTSOS_HEAP_BLOCK_SIZE;
  return heap_malloc_blocks(heap, total_blocks);
}

void heap_free(struct heap *heap, void *ptr) { heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr)); }