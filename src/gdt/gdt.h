#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*
First flag is reserved (always zero)
Gr: Granularity flag, indicates the size the Limit value is scaled by. If clear (0), the Limit is in 1 Byte blocks (byte granularity). If set
(1), the Limit is in 4 KiB blocks (page granularity).
Sz: Size flag. If clear (0), the descriptor defines a 16-bit protected mode segment. If
set (1) it defines a 32-bit protected mode segment. A GDT can have both 16-bit and 32-bit selectors at once.
L: Long-mode code flag. If set (1),
the descriptor defines a 64-bit code segment. When set, Sz should always be clear. For any other type of segment (other code types or any data
segment), it should be clear (0).
*/
#define GDT_FLAGS_4KB_BLOCKS        0b1000
#define GDT_FLAGS_32BIT_SEGMENT     0b0100
#define GDT_FLAGS_LONG_MODE         0b0010

#define GDT_MAX_LIMIT_FOR_BYTE_MODE 0xFFFFF + 1

struct gdt_raw {
  uint16_t limit_low_16bits;
  uint16_t base_low_16bits;
  uint8_t base_mid_8bits;
  uint8_t access;
  uint8_t limit_high_and_flags;
  uint8_t base_high_8bits;
} __attribute__((packed));

/* Access byte:
 * 7   6   5	 4   3   2   1   0
 * Pr	 Privl	 S   Ex  DC  RW  Ac
 *
 * Pr: Present bit. Allows an entry to refer to a valid segment. Must be set (1) for any valid segment.
 * Privl: Descriptor privilege level field. Contains the CPU Privilege level of the segment. 0 = highest privilege (kernel), 3 = lowest privilege (user applications). 
 * S: Descriptor type bit. If clear (0) the descriptor defines a system segment (eg. a Task State Segment). If set (1) it defines a code or data segment. 
 * Ex: Executable bit. If clear (0) the descriptor defines a data segment. If set (1) it defines a code segment which can be executed from. 
 * DC: Direction bit/Conforming bit. 
 *   For data selectors: Direction bit. If clear (0) the segment grows up. If set (1) the segment grows down, ie. the Offset has to be greater than the Limit. 
 *   For code selectors: Conforming bit. If clear (0) code in this segment can only be executed from the ring set in Privl. If set (1) code in this segment can be executed from an equal or lower privilege level. 
 * RW: Readable bit/Writable bit. 
 *   For code segments: Readable bit. If clear (0), read access for this segment is not allowed. If set (1) read access is allowed. Write access is never allowed for code segments. 
 *   For data segments: Writeable bit. If clear (0), write access for this segment is not allowed. If set (1) write access is allowed. Read access is always allowed for data segments. 
 * Ac: Accessed bit. Best left clear(0), the CPU will set it when the segment is accessed.
 */
#define GDT_SEG_ACESS(x)       ((x))
#define GDT_SEG_RW(x)          ((x) << 1)
#define GDT_SEG_DC(x)          ((x) << 2)
#define GDT_SEG_EX(x)          ((x) << 3)
#define GDT_SEG_DESC(x)        ((x) << 4)
#define GDT_SEG_PRIV(x)        (((x)&0b111) << 5)
#define GDT_SEG_PRES(x)        ((x) << 7)

#define GDT_TSS_TSSLDT(x)      ((x))
#define GDT_TSS_BUSY(x)        ((x) << 1)
//#define GDT_TSS_ZERO(x)      ((x) << 2)
#define GDT_TSS_32BIT(x)       ((x) << 3)
#define GDT_TSS_TSSLDT2(x)     ((x) << 4)
#define GDT_TSS_PRIV(x)        (((x)&0b111) << 5)
#define GDT_TSS_PRES(x)        ((x) << 7)

// GDT hardcoded offsets
#define GDT_ENTRY_SIZE         (2 * sizeof(uint32_t))
#define GDT_KERNEL_CODE_OFFSET (1 * GDT_ENTRY_SIZE)
#define GDT_KERNEL_DATA_OFFSET (2 * GDT_ENTRY_SIZE)
#define GDT_USER_CODE_OFFSET   (3 * GDT_ENTRY_SIZE)
#define GDT_USER_DATA_OFFSET   (4 * GDT_ENTRY_SIZE)
#define GDT_TSS_OFFSET         (5 * GDT_ENTRY_SIZE)

#define GDT_PRIVILEGE_RING_0   0
#define GDT_PRIVILEGE_RING_3   3
#define GDT_TI_BIT(x)          ((x)) << 3

struct gdt {
  uint32_t base;
  uint32_t limit;
  uint8_t type;
};

void gdt_load(struct gdt_raw *gdt_raw, int size);
void get_raw_gdt_struct(struct gdt_raw *gdt_raw, const struct gdt *gdt, int total_entires);
#endif