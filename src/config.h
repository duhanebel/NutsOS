#ifndef CONFIG_H
#define CONFIG_H
#include "gdt/gdt.h"

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

#define NUTSOS_PROGRAM_VIRTUAL_ADDRESS             0x00400000
#define NUTSOS_USER_PROGRAM_STACK_SIZE             1024 * 16
#define NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START 0x003FF000
#define NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END \
  (NUTSOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START - NUTSOS_USER_PROGRAM_STACK_SIZE) // stack grows downwards

// HID IO
#define NUTSOS_KEYBOARD_BUFFER_SIZE 1024

#endif