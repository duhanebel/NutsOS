#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt_desc {
  uint16_t offset_low;  // Offset bits 0 - 15
  uint16_t selector;    // Selector thats in our GDT
  uint8_t unused;       // Does nothing, unused set to zero
  uint8_t type_attr;    // Descriptor type and attributes
  uint16_t offset_high; // Offset bits 16-31
} __attribute__((packed));

struct idtr_desc {
  uint16_t limit; // Size of descriptor table -1
  uint32_t base;  // Base address of the start of the interrupt descriptor table
} __attribute__((packed));

#define IDT_OFFSET_LOW(v)  ((uint32_t)(v)&0x0000FFFF)
#define IDT_OFFSET_HIGH(v) ((uint32_t)(v) >> 16)
#define IDT_ATTR_RING(v)   (((v)&0b00000011) << 5)
#define IDT_ATTR_INT32     0b00001110
#define IDT_ATTR_TRA32     0b00001111
#define IDT_ATTR_PRESENT   0b10000000

// Represent the process state when an int is called
struct interrupt_frame {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t reserved;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  uint32_t ip;
  uint32_t cs;
  uint32_t flags;
  uint32_t esp;
  uint32_t ss;
} __attribute__((packed));



void idt_init();
// defined in idt.asm
void enable_interrupts();
// defined in idt.asm
void disable_interrupts();


#endif
