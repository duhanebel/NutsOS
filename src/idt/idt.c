#include "idt.h"
#include "../config.h"
#include "io/io.h"
#include "kernel.h"
#include "memory/memory.h"
#include "terminal/terminal.h"

struct idt_desc idt_descriptors[NUTSOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

#define UINT32_LOW(i)  ((i) && 0x0000FFFF)
#define UINT32_HIGH(i) ((i) >> 16)

extern void idt_load(struct idtr_desc *ptr);
extern void int21h();
extern void no_interrupt();

void int21h_handler()
{
  print("Keyboard pressed!\n");
  outb(0x20, 0x20);
}

void no_interrupt_handler()
{
  outb(0x20, 0x20);
}

void idt_zero()
{
  print("Divide by zero error\n");
}

void idt_set(int interrupt_no, void *address)
{
  struct idt_desc *desc = &idt_descriptors[interrupt_no];
  desc->offset_1 = IDT_OFFSET_LOW(address); //(uint32_t) address & 0x0000ffff;
  desc->selector = KERNEL_CODE_SELECTOR;
  desc->unused = 0x00;

  // 32bit interrupt, accessible from ring 3
  desc->type_attr = IDT_ATTR_PRESENT | IDT_ATTR_INT32 | IDT_ATTR_RING(3);
  desc->offset_2 = IDT_OFFSET_HIGH(address); //(uint32_t) address >> 16;
}

void idt_init()
{
  memset(idt_descriptors, 0, sizeof(idt_descriptors));
  idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
  idtr_descriptor.base = (uint32_t)idt_descriptors;

  for (int i = 0; i < NUTSOS_TOTAL_INTERRUPTS; i++) {
    idt_set(i, no_interrupt);
  }

  idt_set(0, idt_zero);
  idt_set(0x21, int21h);

  // Load the interrupt descriptor table
  idt_load(&idtr_descriptor);
}