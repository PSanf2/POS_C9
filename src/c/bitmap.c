#include <bitmap.h>

static u8int bitarray[131072]; // that's the size of the array in bytes! (1MB)
static u32int num_frames;
static u32int max_index;

void bitmap_initialize(u32int frames)
{
	// initialize the bitarray to all zeros
	memset((u8int *) bitarray, 0, 131072);
	
	// save the number of frames (this will also tell me where to stop when searching for free frames)
	num_frames = frames;
	
	// the max index will tell me the last place i could possibly set or clear a bit in the array.
	max_index = (num_frames / 8) - 1;
}

// the frame address is the page aligned physical address for the frame in question
void set_frame(u32int frame_addr)
{
	// in order to figure out which bit i need to set i'll need to...
		// divide the frame address by 0x1000 to figure out which number frame it is
		// divide that number by 8 to figure out which byte the bit is in
		// do frame address mod 8 to figure out which bit in the byte i should set
		// do bitset[index] |= (0x1 << offset) to set the bit
	
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 8;
	u32int offset = frame % 8;
	bitarray[index] |= (0x1 << offset);
}

void clear_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 8;
	u32int offset = frame % 8;
	bitarray[index] &= ~(0x1 << offset);
}

u32int test_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 8;
	u32int offset = frame % 8;
	return (bitarray[index] & (0x1 << offset));
}

u32int first_free()
{
	// for each byte on the bitarray
	for (u32int i = 0; i < max_index; i++)
	{
		// if there's a bit that's not set
		if (bitarray[i] != 0xFF)
		{
			// for each bit in that byte
			for (u32int j = 0; j < 8; j++)
			{
				// if the bit is not set
				u32int test = (0x1 << j);
				if (!(bitarray[i] & test))
				{
					// return the physical address of that page grame
					return ((i * 8) + j) * 0x1000;
				}
			}
		}
	}
	// if i get down to here then it means that there was no free frame found
	// i need to have some way to return an error code
	// because my memory address for a 4GB system run from 0x0 - 0xFFFFFFFF i'll
	// return 0xFFFFFFFF because that'd be an invalid memory address.
	return 0xFFFFFFFF;
}
