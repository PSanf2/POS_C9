#include <pmm.h>

extern u32int BootPageDirectory;
extern u32int start;
extern u32int end;

u32int boot_page_dir_addr = (u32int) &BootPageDirectory;
u32int kernel_start = (u32int) &start;
u32int kernel_end = (u32int) &end;

bitmap_type *pmm_frames = 0x0;

void pmm_initialize(struct multiboot *mboot_ptr)
{
	// For a 32-bit system there is a maximum possible 4 GB of memory.
	// This give us 4,096 MB of memory, or 4,194,304 KB
	// 41,94,304 / 4 = 1,048,576 frames.
	// 1,048,576 frames will require 1,048,576 bits to keep track of on
	// a bitmap. That's a theoretical maximum  of 1 MB of memory needed
	// for the bitmap.
	
	// if the physical memory manager has already been initialized
	if ((u32int) pmm_frames != 0x0)
	{
		// stop what i'm doing
		return;
	}
	
	// figure out how much physical memory I have to manage
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	u32int mem_in_kb = mem_in_mb * 1024;
	u32int num_frames = mem_in_kb / 4;
	
	// figure out where i'll be putting some things
	u32int frames_addr = ((kernel_end & ~(0xFFF)) + 0x1000) - sizeof(bitmap_type);
	u32int bitmap_addr = ((kernel_end & ~(0xFFF)) + 0x1000);
	u32int bitmap_size = num_frames / 8;
	
	// create a pointer to that place in memory
	pmm_frames = (bitmap_type *) frames_addr;
	
	// make a pointer to the start of the bitmap
	pmm_frames->addr = (u8int *) bitmap_addr;
	
	// set the size of the bitmap
	pmm_frames->bytes = bitmap_size;
	
	// clear out everything on the bitmap to make sure there's no garbage in there
	clear_all_bits(pmm_frames);
	
	/*
	 * I need to figure out which frames are actually in use.
	 * Before the kernel is started the boot script enables paging with
	 * 4 MB pages. With this configuration I don't have any page tables.
	 * Each entry on the page directory represents 4 MB of memory that's
	 * mapped. For the sake of simplicity, and sanity, I'm going to
	 * assume that the kernel (and bitmap) always takes up that 4 MB of
	 * space. The truth is that the kernel and bitmap will always RESIDE
	 * in that 4 MB of space.
	 */
	
	// make a pointer to the page directory
	u32int *boot_page_dir = (u32int *) boot_page_dir_addr;
	
	// for each entry in the page directory
	for (u32int i = 0; i < 1024; i++)
	{
		// if there's a 4 MB page present
		if (boot_page_dir[i] & 0x1)
		{
			// figure out the physical address for that 4 MB of memory
			u32int phys_addr = boot_page_dir[i] & ~(0xFFF);
			
			// for each 4 KB page in that 4 MB of memory
			for (u32int j = 0; j < 1024; j++)
			{
				// figure out the physical address for that frame
				u32int four_k_frame_addr = j * 0x1000 + phys_addr;
				
				// figure out which bit on the bitmap represents that frame
				u32int bit_on_bitmap = four_k_frame_addr / 0x1000;
				// yeah, it's "dumb" but it's correct.
				
				// set that bit on the bitmap
				set_bit(pmm_frames, bit_on_bitmap);
			}
		}
	}
}

u32int alloc_frame()
{
	// i need to make sure the memory manager has been initialized
	// if the memory manager has not been initialized
	if ((u32int) pmm_frames == 0x0)
	{
		// throw a fit, and refuse to play any more.
		put_str("\nPhysical memory manager has not been initialized!");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	// set up the default return value. if there aren't any free frames
	// then i'll return 0xFFFF FFFF as an error value. This is because
	// on a 4GB system it's an invalid memory address.
	u32int result = 0xFFFFFFFF;
	
	// if there is a free frame
	if (any_bit_clear(pmm_frames))
	{
		// figure out the first free bit on the bitmap.
		u32int first_clear_bit = find_first_clear_bit(pmm_frames);
		
		// set that bit
		set_bit(pmm_frames, first_clear_bit);
		
		// figure out the address for that frame.
		u32int frame_addr = first_clear_bit * 0x1000;
		
		// that's the result
		result = frame_addr;
	}
	
	return result;
}

void free_frame(u32int addr)
{
	// i need to make sure the memory manager has been initialized
	// if the memory manager has not been initialized
	if ((u32int) pmm_frames == 0x0)
	{
		// throw a fit, and refuse to play any more.
		put_str("\nPhysical memory manager has not been initialized!");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	// sanitize the address to line it up with a frame address
	u32int aligned_addr = addr & ~(0xFFF);
	
	// i need to figure out which bit represents that address
	u32int bit_on_bitmap = aligned_addr / 0x1000;
	
	// clear the bit on the bitmap
	clear_bit(pmm_frames, bit_on_bitmap);
	
}
