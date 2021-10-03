#include "kernel.h"
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
  kernel_chunk = paging_chunk_new(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

  // Switch to kernel paging chunk
  paging_switch(paging_chunk_get_directory(kernel_chunk));

  char *ptr = kzalloc(4096);
  paging_set(paging_chunk_get_directory(kernel_chunk), (void *)0x1000,
             (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);

  // Enable paging
  enable_paging();

  char *ptr2 = (char *)0x1000;
  ptr2[0] = 'A';
  ptr2[1] = 'B';
  print(ptr2);

  print(ptr);

  // Enable the system interrupts
  enable_interrupts();
}
