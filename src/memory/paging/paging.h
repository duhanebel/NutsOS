#ifndef PAGING_H
#define PAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGING_CACHE_DISABLED          0b00010000
#define PAGING_WRITE_THROUGH           0b00001000
#define PAGING_ACCESS_FROM_ALL         0b00000100
#define PAGING_IS_WRITEABLE            0b00000010
#define PAGING_IS_PRESENT              0b00000001

// Defining a directroy of 1024 entry, each with 1024 location describing 4k each, we manage to cover 4GB of RAM
// reference: https://wiki.osdev.org/Paging
#define PAGING_TOTAL_DIR_ENTRIES       1024
#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_PAGE_SIZE               4096

typedef uint32_t paging_entry;
typedef uint32_t paging_dir;

struct paging_chunk {
  paging_dir *directory_entry;
  int dir_count;
  int entries_count;
};

// Creates a new paging directory containing dir_entries entries, each with
// page_entries describing PAGING_PAGE_SIZE bytes of RAM
struct paging_chunk *paging_chunk_new(int dir_entries, int page_entries, uint8_t flags);

// Delete a paging chunk
void paging_chunk_free(struct paging_chunk *chunk);

// Switches the paging directory in use
void paging_switch(paging_dir *directory);

// Enable paging on the CPU
void enable_paging();

// Sets a page for a specific virtual address (PAGING_PAGE_SIZE aligned)
int paging_set(paging_dir *directory, void *virt, paging_entry pdesc);

// Returns true if addr is aligned to PAGING_PAGE_SIZE
bool paging_is_aligned(void *addr);
void *paging_align_address(void *ptr);

// Returns paging_dir of a paging_chunk
paging_dir *paging_chunk_get_directory(struct paging_chunk *chunk);

int paging_map_to(uint32_t *directory, void *virt, void *phys, void *phys_end, int flags);


#endif