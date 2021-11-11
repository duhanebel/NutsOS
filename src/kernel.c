#include "kernel.h"
#include "disk/disk.h"
#include "disk/stream.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "terminal/terminal.h"
#include <stddef.h>
#include <stdint.h>

static struct paging_chunk *kernel_chunk = 0;
void kmain()
{
  terminal_initialize();
  print("Hello world!\ntest");

  // Initialize the heap
  kheap_init();

  // Initialize the interrupt descriptor table
  idt_init();

  // Setup paging
  kernel_chunk = paging_chunk_new(
      PAGING_TOTAL_DIR_ENTRIES, PAGING_TOTAL_ENTRIES_PER_TABLE, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

  // Switch to kernel paging chunk
  paging_switch(paging_chunk_get_directory(kernel_chunk));

  // Enable paging
  enable_paging();

  // Search and initialize the disks
  disk_search_and_init();

  // Enable the system interrupts
  enable_interrupts();

  struct disk_stream *stream = diskstream_new(0);
  diskstream_seek(stream, 0x130);
  char buf[1000];
  // unsigned char c = 0;
  // diskstream_read(stream, &c, 1);
  diskstream_read(stream, &buf, 600);
  while (1) {}
}
