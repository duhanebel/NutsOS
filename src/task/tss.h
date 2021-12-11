#ifndef TASKSWITCHSEGMENT_H
#define TASKSWITCHSEGMENT_H

#include <stdint.h>
struct tss {
  uint32_t link;
  uint32_t esp0; // Kernel stack pointer
  uint32_t ss0;  // Kernel stack segment (lower 16bits)

  // Mostly ignored as multitasking is implemented in software
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldtr;
  uint32_t iopb;
} __attribute__((packed));

// Defined in tss.asm
extern void tss_load(int tss_segment);

#endif