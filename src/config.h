#ifndef CONFIG_H
#define CONFIG_H
#include "gdt/gdt.h"

#define KERNEL_CODE_SELECTOR                       0x08
#define KERNEL_DATA_SELECTOR                       0x10

#define NUTSOS_TOTAL_INTERRUPTS                    512

// 512MB heap size
#define NUTSOS_HEAP_SIZE_BYTES                     512 * (1024 * 1024)
#define NUTSOS_HEAP_BLOCK_SIZE                     4096
#define NUTSOS_HEAP_ADDRESS                        0x01000000
#define NUTSOS_HEAP_TABLE_ADDRESS                  0x00007E00

// Disk
#define NUTSOS_SECTOR_SIZE                         512
#define NUTSOS_MAX_PATH                            256

// FS
#define NUTSOS_MAX_FILESYSTEMS                     16
#define NUTSOS_MAX_FILE_DESCRIPTORS                1024

// Processes
#define NUTSOS_MAX_PROCESSES                       1024

#define NUTSOS_PROGRAM_VIRTUAL_ADDRESS             0x400000
#define NUTSOS_USER_PROGRAM_STACK_SIZE             1024 * 16
#define NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START 0x3FF000
#define NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END   (NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START - NUTSOS_USER_PROGRAM_STACK_SIZE)

// Memory
#define NUTSOS_GDT_ENTRY_SIZE                      (2 * sizeof(uint32_t))
#define NUTSOS_GDT_KERNEL_CODE_OFFSET              (1 * NUTSOS_GDT_ENTRY_SIZE)
#define NUTSOS_GDT_KERNEL_DATA_OFFSET              (2 * NUTSOS_GDT_ENTRY_SIZE)
#define NUTSOS_GDT_USER_CODE_OFFSET                (3 * NUTSOS_GDT_ENTRY_SIZE)
#define NUTSOS_GDT_USER_DATA_OFFSET                (4 * NUTSOS_GDT_ENTRY_SIZE)
#define NUTSOS_GDT_TSS_OFFSET                      (5 * NUTSOS_GDT_ENTRY_SIZE)

#endif