#include <bitmap.h>

boolean test_bit(bitmap_type *bitmap, u32int bit)
{
	return (boolean) ((bitmap->addr[bit / 8] & (0x1 << (bit % 8))) != 0);
}

void set_bit(bitmap_type *bitmap, u32int bit)
{
	bitmap->addr[bit / 8] |= (0x1 << (bit % 8));
}

void clear_bit(bitmap_type *bitmap, u32int bit)
{
	bitmap->addr[bit / 8] &= ~(0x1 << (bit % 8));
}

u32int find_first_set_bit(bitmap_type *bitmap)
{
	for (u32int i = 0; i < bitmap->bytes; i++)
	{
		if (bitmap->addr[i] != 0x0)
		{
			for (u8int j = 0; j < 8; j++)
			{
				if ((bitmap->addr[i] & (0x1 << j)))
				{
					return ((i * 8) + j);
				}
			}
		}
	}
	return 0;
}

u32int find_first_clear_bit(bitmap_type *bitmap)
{
	for (u32int i = 0; i < bitmap->bytes; i++)
	{
		if (bitmap->addr[i] != 0xFF)
		{
			for (u8int j = 0; j < 8; j++)
			{
				if (!(bitmap->addr[i] & (0x1 << j)))
				{
					return ((i * 8) + j);
				}
			}
		}
	}
	return 0;
}

void clear_all_bits(bitmap_type *bitmap)
{
	memset((u8int *) bitmap->addr, 0x0, bitmap->bytes);
}

void set_all_bits(bitmap_type *bitmap)
{
	memset((u8int *) bitmap->addr, 0xFF, bitmap->bytes);
}

boolean any_bit_clear(bitmap_type *bitmap)
{
	for (u32int i = 0; i < bitmap->bytes; i++)
	{
		if (bitmap->addr[i] != 0xFF)
		{
			return TRUE;
		}
	}
	return FALSE;
}

boolean any_bit_set(bitmap_type *bitmap)
{
	for (u32int i = 0; i < bitmap->bytes; i++)
	{
		if (bitmap->addr[i] != 0x0)
		{
			return TRUE;
		}
	}
	return FALSE;
}
