#include "kernel.h"
#include "disk/disk.h"
#include "disk/stream.h"
#include "fs/file.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "terminal/terminal.h"
#include <stddef.h>
#include <stdint.h>

static struct paging_chunk *kernel_chunk = 0;

void panic(const char *msg)
{
  print(msg);
  while (1) {}
}

void kmain()
{
  terminal_initialize();
  print("Hello world!\ntest\n\n");
  kprintf("dddee- %s%%%s\n", "Jellow!", "Test!");

  // Initialize the heap
  kheap_init();

  // Initialize filesystems
  fs_init();

  // Search and initialize the disks
  disk_search_and_init();

  // Initialize the interrupt descriptor table
  idt_init();

  // Setup paging
  kernel_chunk = paging_chunk_new(PAGING_TOTAL_DIR_ENTRIES,
                                  PAGING_TOTAL_ENTRIES_PER_TABLE,
                                  PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

  // Switch to kernel paging chunk
  paging_switch(paging_chunk_get_directory(kernel_chunk));

  // Enable paging
  enable_paging();

  // Enable the system interrupts
  enable_interrupts();

  struct file_descriptor *fd = fopen("0:/test_dir/hello.txt", "r");
  if (fd) {
    print("We opened hello.txt!!!\n");
    struct file_stat stat;
    fstat(fd->index, &stat);

    char *buf = kzalloc(stat.filesize);
    fread(buf, stat.filesize, 1, fd->index);
    buf[stat.filesize] = 0x00;
    print(buf);
    kfree(buf);
  } else {
    print("Can't open hello.txt\n");
  }
  while (1) {}
}
