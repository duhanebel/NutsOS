#include "kernel.h"
#include "config.h"
#include "disk/disk.h"
#include "disk/stream.h"
#include "error.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "task/process.h"
#include "task/task.h"
#include "task/tss.h"
#include "terminal/terminal.h"
#include <stddef.h>
#include <stdint.h>

static struct paging_chunk *kernel_chunk = 0;

void panic(const char *msg)
{
  print(msg);
  while (1) {}
}

#define GDT_SEGMENTS_COUNT 6
struct tss tss;
struct gdt_raw gdt_raw[GDT_SEGMENTS_COUNT];
struct gdt gdt[GDT_SEGMENTS_COUNT] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                                                                      // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_SEG_RW(1) | GDT_SEG_EX(1) | GDT_SEG_DESC(1) | GDT_SEG_PRES(1)},   // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_SEG_RW(1) | GDT_SEG_DESC(1) | GDT_SEG_PRES(1)},                   // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_SEG_DESC(1) | GDT_SEG_EX(1) | GDT_SEG_PRIV(3) | GDT_SEG_PRES(1)}, // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = GDT_SEG_DESC(1) | GDT_SEG_RW(1) | GDT_SEG_PRIV(3) | GDT_SEG_PRES(1)}, // User data segment
    {.base = (uint32_t)&tss, .limit = sizeof(tss), .type = 0xE9}};

void kmain()
{
  terminal_initialize();

  struct gdt *thegdt = gdt;
  // setup the GDT
  memset(gdt_raw, 0x00, sizeof(gdt_raw));
  get_raw_gdt_struct(gdt_raw, thegdt, GDT_SEGMENTS_COUNT);
  gdt_load(gdt_raw, sizeof(gdt_raw));

  // Initialize the heap
  kheap_init();

  // Initialize filesystems
  fs_init();

  // Search and initialize the disks
  disk_search_and_init();

  // Initialize the interrupt descriptor table
  idt_init();

  // Setup the TSS
  memset(&tss, 0x00, sizeof(tss));
  tss.esp0 = 0x600000;
  tss.ss0 = KERNEL_DATA_SELECTOR;

  // Load the TSS
  tss_load(NUTSOS_GDT_TSS_OFFSET);

  // Setup paging
  kernel_chunk = paging_chunk_new(PAGING_TOTAL_DIR_ENTRIES,
                                  PAGING_TOTAL_ENTRIES_PER_TABLE,
                                  PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

  // Switch to kernel paging chunk
  paging_switch(paging_chunk_get_directory(kernel_chunk));

  // Enable paging
  enable_paging();

  // Enable the system interrupts
  //enable_interrupts();

  // struct file_descriptor *fd = fopen("0:/test_dir/hello.txt", "r");
  // if (fd) {
  //   print("We opened hello.txt!!!\n");
  //   struct file_stat stat;
  //   fstat(fd->index, &stat);

  //   char *buf = kzalloc(stat.filesize);
  //   fread(buf, stat.filesize, 1, fd->index);
  //   buf[stat.filesize] = 0x00;
  //   print(buf);
  //   kfree(buf);
  // } else {
  //   print("Can't open hello.txt\n");
  // }
  struct process *process = NULL;
  int res = process_load("0:/bin/empty.bin", &process);
  if (ISERR(res)) {
    panic("Unable to load first task!");
  }
  task_run_as_task0(process->task);
  while (1) {}
}
