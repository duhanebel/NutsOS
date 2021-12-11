#include "kernel.h"
#include "config.h"
#include "disk/disk.h"
#include "disk/stream.h"
#include "error.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "io/io.h"
#include "isr80h/isr80h.h"
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
  print("PANIC!!");
  print(msg);
  while (1) {}
}

void kprint(const char *msg) {
  print("[K] ");
  print(msg);
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
    {.base = (uint32_t)&tss,
     .limit = sizeof(tss) - 1,
     .type = GDT_TSS_PRIV(3) | GDT_TSS_PRES(1) | GDT_TSS_32BIT(1) | GDT_TSS_TSSLDT(1)}}; // TSS

void kmain()
{
  terminal_initialize();
  kprint("Terminal initialized.\n");

  // setup the GDT
  kprint("Setting up GDT...");

  get_raw_gdt_struct(gdt_raw, gdt, GDT_SEGMENTS_COUNT);
  struct gdt_raw *t = gdt_raw;
  struct tss *s = &tss;
  gdt_load(t, sizeof(gdt_raw));
  kprint(" done\n");

  // Initialize the heap
  kprint("Initializing kernel heap...");
  kheap_init();
  kprint(" done\n");

  // Initialize filesystems
  kprint("Initializing file systems...");
  fs_init();
  kprint(" done\n");

  // Search and initialize the disks
  kprint("Searching for disks...");
  disk_search_and_init();
  kprint(" done\n");

  // Initialize the interrupt descriptor table
  kprint("Initializing interrupts...");
  idt_init();
  kprint(" done\n");

  // Setup the TSS
  kprint("Initializing TSS...");
  memset(s, 0x00, sizeof(tss));
  tss.ss0 = GDT_KERNEL_DATA_OFFSET;
  tss.esp0 = 0x00600000;

  // Load the TSS
  tss_load(GDT_TSS_OFFSET);
  kprint(" done\n");

  // Setup paging
  kprint("Setting up paging...");
  kernel_chunk = paging_chunk_new(PAGING_TOTAL_DIR_ENTRIES,
                                  PAGING_TOTAL_ENTRIES_PER_TABLE,
                                  PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

  // Switch to kernel paging chunk
  paging_switch(paging_chunk_get_directory(kernel_chunk));

  // Enable paging
  enable_paging();
  kprint(" done\n");

  // Register the kernel commands
  kprint("Registering system call interrupts...");
  isr80h_register_commands();
  kprint(" done\n");

  kprint("Starting first process...\n\n");
  struct process *process = NULL;
  int res = process_load("0:/bin/empty.bin", &process);
  if (ISERR(res)) {
    panic("Unable to load first task!");
  }
  //print("Running process 0...");
  task_run_as_task0(process->task);

  panic("Process 0 terminated!");
  while (1) {}
}
