#include "gdt.h"
#include "kernel.h"

static void encode_gdt_entry(uint8_t *target, struct gdt source)
{
  if ((source.limit > 65536) && ((source.limit & 0xFFF) != 0xFFF)) {
    panic("encode_gdt_entry: invalid argument\n"); // TODO use __FUNCTION__
  }

  target[6] = 0x40;
  if (source.limit > 65536) {
    source.limit = source.limit >> 12;
    target[6] = 0xC0;
  }

  // Encodes the limit
  target[0] = source.limit & 0xFF;
  target[1] = (source.limit >> 8) & 0xFF;
  target[6] |= (source.limit >> 16) & 0x0F;

  // Encode the base
  target[2] = source.base & 0xFF;
  target[3] = (source.base >> 8) & 0xFF;
  target[4] = (source.base >> 16) & 0xFF;
  target[7] = (source.base >> 24) & 0xFF;

  // Set the type
  target[5] = source.type;
}

void get_raw_gdt_struct(struct gdt_raw *gdt_raw, const struct gdt *gdt, int total_entires)
{
  for (int i = 0; i < total_entires; i++) {
    encode_gdt_entry((uint8_t *)&gdt_raw[i], gdt[i]);
  }
}