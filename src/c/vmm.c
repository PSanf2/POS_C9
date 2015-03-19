#include <vmm.h>

extern u32int *page_directory;

static u32int *bitmap;

/*
 * The virtual memory manager will be responsible for allocating, and freeing
 * virtual address space in 4KB chunks. I'll be using a bitmap to keep track
 * of which pages of virtual memory are allocated, or available. Since paging
 * has already been initialized I don't need to worry about setting up any
 * page tables, or page directories. I will need to itterate over the page
 * directory when I initialize the memory manager to figure out which pages
 * of virtual addresses are in use so I can mark them as such in the bitmap.
 * Since I'll be working with the full 4GB worth of virtual address space the
 * memory manager will need to maintain a bitmap of 4MB.
 */

void vmm_initialize(u32int bitmap_addr)
{
	
	// make a pointer to the place where i'll be storing my bitmap
	bitmap = (u32int *) bitmap_addr;
	
	// clear out the 1MB of memory that i'll need for the bitmap.
	memset((u8int *) bitmap, 0, 131072);
	
	// start going over the page directory to see which virtual addresses are in use
	
	/*
	 * I'll need to calculate the used virtual addresses off of the page
	 * directory index, and the page table index.
	 * page dir indx << 22 = first 12 bits of the address
	 * page table indx << 12 = next 12 bytes
	 * add those together and i'll get the page address being used
	 */
	
	// for each entry on the page directory
	for (u32int i = 0; i < 1024; i++)
	{
		// if there's something present on that page directory entry
		if (page_directory[i] & 0x1)
		{
			// make a pointer to the page table (recursive mapping, ftw)
			u32int table_addr_base = 0xFFC00000;
			u32int table_addr_offset = i << 12;
			u32int page_table_virt_addr = table_addr_base + table_addr_offset;
			
			u32int *page_table_ptr;
			page_table_ptr = (u32int *) page_table_virt_addr;
			
			// for each entry on the page table
			for (u32int j = 0; j < 1024; j++)
			{
				// if there's something on that page table entry
				if (page_table_ptr[j] & 0x1)
				{
					// figure out the virtual address being used
					u32int addr_base = i << 22;
					u32int addr_offset = j << 12;
					u32int virt_addr = addr_base + addr_offset;
					
					// set the bit for that page on the bitmap
					set_page(virt_addr);
				}
			}
		}
	}
	
}

void set_page(u32int virt_addr)
{
	u32int page = virt_addr / 0x1000;
	u32int index = page / 64;
	u32int offset = page % 8;
	bitmap[index] |= (0x1 << offset);
}

void clear_page(u32int virt_addr)
{
	u32int page = virt_addr / 0x1000;
	u32int index = page / 64;
	u32int offset = page % 8;
	bitmap[index] &= (0x1 << offset);
}

u32int test_page(u32int virt_addr)
{
	u32int page = virt_addr / 0x1000;
	u32int index = page / 64;
	u32int offset = page % 8;
	return (bitmap[index] & (0x1 << offset));
}

// this may eventually run into an issue where the page fault interrupt handler tries to
// create a page directory or page table at a place that just-so-happens
// to be the same place where i try to create an allocation. the paging
// system is blind, and don't give a crap which virtual addresses it's
// using for page tables. when it needs a new page table it just creates
// one somewhere. if i try to allocate that spot in virtual memory, then
// when i first attempt to use it the page fault handler could decide
// to suddenly create a page table there, and writing to it will cause
// me to overwrite the information needed for the page table. i need to
// have some way to poke the virtual address that i'm wanting to allocate,
// then check it to see if it's still available (hasn't suddenly been used
// for a page table) before attempting to allocate it. at the very worst
// the system will use 4MB+1KB for all of the kernel level paging structures
// so i should be safely able to eventually find some unallocated virtual
// address. the worst case scenario would see all of the memory get mapped
// in a runaway recursion/poke loop, but that'll eventually run into a stop
// condition once all possible page tables had been created.

// i'll need to periodically repopulate the bitmap to make sure i'm
// staying in sync with what the paging stuff is doing.

// this thinking is probably wrong. from what i'm seeing from a test
// everything set up by the paging initilization function is mapped
// to addresses above 0xC0000000. I should be able to play around in
// the lower 3G of address space w/o any issue.

// that thinking is kinda wrong. it depends on where i'm trying to allocate memory
// if it's below 3G (in user space) then i have nothing to worry about.
// that only comes in to play if i'm wanting to screw with memory above the 3G line (kernel space)

u32int *kmalloc(u32int num_pages)
{
	
}

void kfree(u32int *virt_addr)
{
	
}











