#include "kernel.h"
#include "disk/disk.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "terminal/terminal.h"
#include <stddef.h>
#include <stdint.h>

static struct paging_chunk *kernel_chunk = 0;
void kmain() {
  terminal_initialize();
  print("Hello world!\ntest");

  // Initialize the heap
  kheap_init();

  // Initialize the interrupt descriptor table
  idt_init();

  // Setup paging
  kernel_chunk = paging_chunk_new(PAGING_TOTAL_DIR_ENTRIES, PAGING_TOTAL_ENTRIES_PER_TABLE,
                                  PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

  // Switch to kernel paging chunk
  paging_switch(paging_chunk_get_directory(kernel_chunk));

  // Enable paging
  enable_paging();

  // Search and initialize the disks
  disk_search_and_init();

  // Enable the system interrupts
  enable_interrupts();
}
