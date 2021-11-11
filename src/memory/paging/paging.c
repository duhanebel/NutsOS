#include "paging.h"
#include "error.h"
#include "memory/heap/kheap.h"

// A entry in either the directory table or the entry table consists of 8 bits of flags
// followed by a 24 4096-byte aligned pointer.
// 31                                         11      9                 0
// |     4-kbyte aligned page pointer         | Avail |      Flags      |
// Where the flags are, for the paging directory:
// S, or 'Page Size' stores the page size for that specific entry. If the bit is set, then pages are 4 MiB in size. Otherwise, they are 4 KiB.
// A, or 'Accessed' is used to discover whether a page has been read or written to. If it has, then the bit is set, otherwise, it is not.
// D, is the 'Cache Disable' bit. If the bit is set, the page will not be cached. Otherwise, it will be.
// W, the controls 'Write-Through' abilities of the page. If the bit is set, write-through caching is enabled. If not, then write-back is
// enabled instead. U, the 'User/Supervisor' bit, controls access to the page based on privilege level. If the bit is set, then the page may be
// accessed by all.
//     bit is not set, however, only the supervisor can access it.
// R, the 'Read/Write' permissions flag.If the bit is set, the page is read / write.Otherwise when it is not set, the page is read - only.
// P, or 'Present'.If the bit is set, the page is actually in physical memory at the moment.For example, when a page is swapped out,
// For the page table:
// C, or Cached, is the 'D' bit from the previous table
// G, or the Global, flag, if set, prevents the TLB from updating the address in its cache if CR3 is reset.
// D, or the Dirty flag, if set, indicates that page has been written to.
// 0, if PAT is supported, shall indicate the memory type. Otherwise, it must be 0.

#define PAGING_ENTRY_GET_POINTER(entry) (uint32_t *)((entry)&0xFFFFF000)
#define PAGING_ENTRY_GET_FLAGS(entry)   ((entry)&0x00000FFF)

// Defined in paging.asm
void paging_load_directory(paging_dir *directory);

// Pointer to the directory in use
static paging_dir *current_directory = 0;

struct paging_chunk *paging_chunk_new(int dir_entries, int page_entries, uint8_t flags)
{
  // Alloc a full directory of entries
  uint32_t *directory = kzalloc(sizeof(paging_dir) * dir_entries);

  // For each one of these, populate the entry and assign it to the directory
  int offset = 0;
  for (int i = 0; i < dir_entries; i++) {
    paging_entry *entry = kzalloc(sizeof(paging_entry) * page_entries);

    // For each entry in the entry table, calculate the real memory it points to.
    // In this case we assign linearly, meaning that virt address 0xXX will map to real address 0xXX
    for (int b = 0; b < page_entries; b++) {
      entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
    }
    // Move the offset to the beginning of the next entry table
    offset += (page_entries * PAGING_PAGE_SIZE);

    // Assign the directory entry (remember the format contains a pointer + flags)
    directory[i] = (uint32_t)entry | (flags | PAGING_IS_WRITEABLE);
  }

  struct paging_chunk *chunk = kzalloc(sizeof(struct paging_chunk));
  chunk->directory_entry = directory;
  chunk->dir_count = dir_entries;
  chunk->entries_count = page_entries;

  return chunk;
}

void paging_switch(paging_dir *directory)
{
  paging_load_directory(directory);
  current_directory = directory;
}

paging_dir *paging_chunk_get_directory(struct paging_chunk *chunk)
{
  return chunk->directory_entry;
}

bool paging_is_aligned(void *addr)
{
  return ((uint32_t)addr % PAGING_PAGE_SIZE) == 0;
}

// Get page directory and entry relative to virtual_address (must be PAGE_SIZE aligned)
int paging_get_indexes(void *virtual_address, uint32_t *directory_index_out, uint32_t *table_index_out)
{
  int res = 0;
  if (!paging_is_aligned(virtual_address)) {
    return -EINVARG;
  }

  *directory_index_out = ((uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
  *table_index_out = ((uint32_t)virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);

  return res;
}

// Set page descriptor for the page related to address virt in directory
int paging_set(paging_dir *directory, void *virt, paging_entry pdesc)
{
  if (!paging_is_aligned(virt)) {
    return -EINVARG;
  }

  uint32_t directory_index = 0;
  uint32_t table_index = 0;
  int res = paging_get_indexes(virt, &directory_index, &table_index);
  if (res < 0) {
    return res;
  }

  // Grab the entry in the table directory
  uint32_t entry = directory[directory_index];
  // Extract the entry-table pointer from the entry
  uint32_t *table = PAGING_ENTRY_GET_POINTER(entry);
  // Set the new value
  table[table_index] = pdesc;

  return 0;
}