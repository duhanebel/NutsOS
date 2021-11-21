#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdt_raw {
  uint16_t segment;
  uint16_t base_low_16bits;
  uint8_t base_mid_8bits;
  uint8_t access;
  uint8_t seg_high_and_flags;
  uint8_t base_hi_8bits;
} __attribute__((packed));

struct gdt {
  uint32_t base;
  uint32_t limit;
  uint8_t type;
};

void gdt_load(struct gdt_raw *gdt_raw, int size);
void gdt_structured_to_gdt(struct gdt_raw *gdt_raw, struct gdt *gdt, int total_entires);
#endif