#include "idt.h"
#include "../config.h"
#include "gdt/gdt.h"
#include "io/io.h"
#include "isr80h/isr80h.h"
#include "kernel.h"
#include "memory/memory.h"
#include "task/task.h"
#include "terminal/terminal.h"

// Defined in idt.asm
extern void idt_load(struct idtr_desc *ptr);
extern void int21h();
extern void no_interrupt();
extern void isr80h_wrapper();

__attribute__((aligned(0x10))) struct idt_desc idt_descriptors[NUTSOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

#define UINT32_LOW(i)  ((i) & 0x0000FFFF)
#define UINT32_HIGH(i) ((i) >> 16)

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

// Set a handler for an interrupt
// interrupt_no:number of the interrupt to set
// address: addres of the function handling the interrupt
void idt_set(int interrupt_no, void *address)
{
  struct idt_desc *desc = &idt_descriptors[interrupt_no];
  desc->offset_low = IDT_OFFSET_LOW(address);
  desc->selector = GDT_KERNEL_CODE_OFFSET;
  desc->unused = 0x00;

  // 32bit interrupt, accessible from ring 3
  desc->type_attr = IDT_ATTR_PRESENT | IDT_ATTR_INT32 | IDT_ATTR_RING(3);
  desc->offset_high = IDT_OFFSET_HIGH(address);
}

// Initialise the idt by creating the idtr_descriptor table and loading it onto the cpi
void idt_init()
{
  memset(idt_descriptors, 0, sizeof(idt_descriptors));
  idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
  idtr_descriptor.base = (uint32_t)idt_descriptors;

  for (int i = 0; i < NUTSOS_TOTAL_INTERRUPTS; i++) {
    idt_set(i, no_interrupt);
  }

  idt_set(0x00, idt_zero);
  idt_set(0x21, int21h);
  idt_set(0x80, isr80h_wrapper);

  // Load the interrupt descriptor table
  idt_load(&idtr_descriptor);
}


// Handler for the INT80 (used for kernel call from user space)
// command: command number
// interrupt_frame: frame of the caller
void *isr80h_handler(int command, struct interrupt_frame *frame)
{
  void *res = 0;
  //kernel_page();
  task_current_save_state(frame);
  res = isr80h_handle_command(command, frame);
  //task_page();
  return res;
}
