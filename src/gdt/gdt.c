#include "gdt.h"
#include "kernel.h"
#include "stdutil/string.h"

static void encode_gdt_entry(struct gdt_raw *target, const struct gdt *source)
{
  //  With 4k granularity, the segment limit is shifted 12 bits
  // left yielding 0xFF000 and the low 12 bits are set to 1.
  // So our gdt.limit needs to have 0xFFF in the last 12 bits when
  // it passes the max size allowed with 1byte mode (0xFFFFF).
  // Any other limit would be invalid.
  // see: https://stackoverflow.com/questions/55968523/bochs-gdt-segment-limit-is-shifted-left-3-times-in-hex-and-0xfff-is-added-is-th
  if ((source->limit > GDT_MAX_LIMIT_FOR_BYTE_MODE) && ((source->limit & 0xFFF) != 0xFFF)) {
    //  if ((source.limit > 65536) && ((source.limit & 0xFFF) != 0xFFF)) {
    char buf[512];
    sprintf(buf, "%s: %s", __FUNCTION__, "invalid argument");
    panic(buf);
  }

  uint8_t flags = GDT_FLAGS_32BIT_SEGMENT;
  uint32_t limit = source->limit;
  // Check if we can fit in 1byte limit or we need to switch to 4k limit
  // addressing mode.
  if (limit > GDT_MAX_LIMIT_FOR_BYTE_MODE) {
    // if (source.limit > 65536) {
    flags |= GDT_FLAGS_4KB_BLOCKS;
    limit >>= 12; // divide limit (in byes) by 4096.
  }

  // Encodes the limit
  target->limit_low_16bits = limit & 0xFFFF;
  // Lower 4 bits of limit are reserved for flags
  target->limit_high_and_flags = ((limit >> 16) & 0x0F) | ((flags << 4) & 0xF0);

  // Encode the base
  target->base_low_16bits = source->base && 0xFFFF;
  target->base_mid_8bits = (source->base >> 16) && 0xFF;
  target->base_hi_8bits = (source->base >> 24) && 0xFF;

  target->access = source->type;
}

void get_raw_gdt_struct(struct gdt_raw *gdt_raw, const struct gdt *gdt, int total_entires)
{
  for (int i = 0; i < total_entires; i++) {
    encode_gdt_entry(&gdt_raw[i], &gdt[i]);
  }
}