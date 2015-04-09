#ifndef __BITMAP_H
#define __BITMAP_H

#include <system.h>

typedef struct bitmap_struct
{
	u8int *addr;
	u32int bytes;
} bitmap_type;

// a bitmap can only have a theoretical maximum size of 536,870,912 bytes (512MB, 0.5GB), or 4,294,967,296 bits
// this is because, with a 32-bit system, we can address the bits from 0x0 to 0xFFFF FFFF
boolean test_bit(bitmap_type *bitmap, u32int bit);
void set_bit(bitmap_type *bitmap, u32int bit);
void clear_bit(bitmap_type *bitmap, u32int bit);
u32int find_first_set_bit(bitmap_type *bitmap);
u32int find_first_clear_bit(bitmap_type *bitmap);
void clear_all_bits(bitmap_type *bitmap);
void set_all_bits(bitmap_type *bitmap);
boolean any_bit_clear(bitmap_type *bitmap);
boolean any_bit_set(bitmap_type *bitmap);

#endif
