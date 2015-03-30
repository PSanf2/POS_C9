#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

u32int kernel_start = (u32int) &start;
u32int kernel_end = (u32int) &end;

page_directory_type kernel_page_directory;
volatile page_directory_type *current_page_directory;

static u8int *frames;
static u32int max_index;

void paging_initialize(struct multiboot *mboot_ptr)
{
	
	// set up the system to use 4KB pages
	
	// i'm going to set up the page directory at the first page aligned
	// address after the kernel.
	
	// gather information
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	u32int mem_in_kb = mem_in_mb * 1024;
	u32int num_frames = mem_in_kb / 4;
	
	u32int kernel_page_dir_virt_addr = (kernel_end & ~(0xFFF)) + 0x1000;
	u32int kernel_page_dir_phys_addr = kernel_page_dir_virt_addr - 0xC0000000;
	
	u32int kernel_page_table_virt_addr = kernel_page_dir_virt_addr + 0x1000;
	u32int kernel_page_table_phys_addr = kernel_page_table_virt_addr - 0xC0000000;
	
	// make a pointer to the place i'll be creating the page directory
	u32int *page_dir_ptr = (u32int *) kernel_page_dir_virt_addr;
	
	// empty out that 4KB of memory
	memset((u8int *) page_dir_ptr, 0, 4096);
	
	// create an empty page directory in there
	for (u32int i = 0; i < 1024; i++)
	{
		page_dir_ptr[i] = 0 | 2;
	}
	
	// make a pointer to the place i'll be creating my page table
	u32int *page_table_ptr = (u32int *) kernel_page_table_virt_addr;
	
	// empty out that 4KB of space
	memset((u8int *) page_table_ptr, 0, 4096);
	
	// create the mappings i'll need for that page table
	u32int phys_addr_ctr = 0x0;
	for (u32int i = 0; i < 1024; i++)
	{
		if (phys_addr_ctr <= kernel_page_table_phys_addr)
		{
			page_table_ptr[i] = phys_addr_ctr | 3;
		}
		else
		{
			page_table_ptr[i] = 0 | 2;
		}
		phys_addr_ctr += 0x1000;
	}
	
	// figure out the page directory index i'll need to work with.
	u32int kernel_page_dir_index = 0xC0000000 >> 22;
	
	// put the physical address of the page table on the page directory at the proper index.
	page_dir_ptr[kernel_page_dir_index] = kernel_page_table_phys_addr | 3;
	
	// set up the recursive mappings
	// FFC0 0000 = page table 0 virt addr
	// FFC0 1000 = page table 1
	// FFC0 2000 = page table 2
	// etc...
	// FFFF F000 = page table 1023
	page_dir_ptr[1023] = kernel_page_dir_phys_addr | 3;
	
	// save the virtual address and physical address of the page directory
	kernel_page_directory.virt_addr = (u32int *) kernel_page_dir_virt_addr;
	kernel_page_directory.phys_addr = kernel_page_dir_phys_addr;
	
	// set up the pointer for the current page directory
	current_page_directory = (page_directory_type *) &kernel_page_directory;
	
	// figure out the new cr4 value i'll need
	u32int new_cr4_val = read_cr4() & ~(0x00000010);
	
	// switch the system over to 4KB paging, and give it the phys addr of the new page dir
	asm volatile (
		"mov %0, %%cr3\n\t"
		"mov %1, %%cr4"
		: /* no outputs */
		: "r" (kernel_page_dir_phys_addr), "r" (new_cr4_val)
	);
	
	// enable paging (just to make sure i've done it)
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
	// set up the bitmap.
	/*
	 * The bitmap will take up a theoretical maximum of 1MB of space
	 * The bitmap will be located in the physical memory right after the kernel page table.
	 * The bitmap will be mapped to the virtual address range of 0xFFB00000 to 0xFFBFFFFF.
	 * The bitmap's page directory will be in PD[1022].
	 */
	
	// figure out the virtual and physical address for the bitmap page table
	u32int bitmap_page_table_virt_addr = kernel_page_table_virt_addr + 0x1000;
	u32int bitmap_page_table_phys_addr = bitmap_page_table_virt_addr - 0xC0000000;
	
	// figure out the virtual and physical address i'll use for the bitmap
	u32int bitmap_virt_addr = 0xFFB00000;
	u32int bitmap_phys_addr = bitmap_page_table_phys_addr + 0x1000;
	
	// figure out the number of frames i'll need for the bitmap
	u32int bytes_needed_for_bitmap = num_frames / 8;
	u32int frames_needed_for_bitmap = bytes_needed_for_bitmap / 0x1000;
	
	if (bytes_needed_for_bitmap % 0x1000 != 0)
	{
		frames_needed_for_bitmap++;
	}
	
	// create a page table for the bitmap.
	// figure out the index on the page table to use
	u32int table_index = (bitmap_page_table_virt_addr >> 12) & 0x3FF;
	
	// set it as present on the kernel page table
	page_table_ptr[table_index] = bitmap_page_table_phys_addr | 3;
	
	// invalidate the buffer for it
	invlpg(bitmap_page_table_virt_addr);
	
	// make a pointer to the bitmap page table
	page_table_ptr = (u32int *) bitmap_page_table_virt_addr;
	
	// clear out the memory in there
	memset((u8int *) page_table_ptr, 0, 4096);
	
	// make a page table in there
	for (u32int i = 0; i < 1024; i++)
	{
		page_table_ptr[i] = 0 | 2;
	}
	
	// put the required pages on the page table
	// this is at a funny offset because the portion of the page directory
	// we're working w/ is in the last 3/4 of the 4MB space it can map.
	u32int bitmap_phys_addr_ctr = bitmap_phys_addr;
	for (u32int i = 768; i < (768 + frames_needed_for_bitmap); i++)
	{
		page_table_ptr[i] = bitmap_phys_addr_ctr | 3;
		bitmap_phys_addr_ctr += 0x1000;
	}
	
	// put the physical address of the bitmap page table on the page directory at the appropriate index.
	page_dir_ptr[1022] = bitmap_page_table_phys_addr | 3;
	
	// make a pointer to the bitmap
	frames = (u8int *) bitmap_virt_addr;
	
	// figure out the max index on that array.
	max_index = (num_frames / 64) - 1;
	
	for (u32int i = 0; i < num_frames; i++)
	{
		clear_frame(i * 0x1000);
	}
	
	// go over the page directory, and page tables, to figure out which frames are being used
	
	// for each entry in the page directory
	for (u32int i = 0; i < 1024; i++)
	{
		// if there's a page table present
		if (page_dir_ptr[i] & 0x1)
		{
			// set the frame for the page table
			set_frame(page_dir_ptr[i] & ~(0xFFF));
			
			// figure out the virtual address for the page table's recursive mapping
			u32int virt_addr_base = 0xFFC00000;
			u32int virt_addr_offset = i << 12;
			u32int page_table_virt_addr = virt_addr_base + virt_addr_offset;
			
			// reset the page_table_ptr to that address
			page_table_ptr = (u32int *) page_table_virt_addr;
			
			// for each entry on the page table
			for (u32int j = 0; j < 1024; j++)
			{
				if (page_table_ptr[j] & 0x1)
				{
					set_frame(page_table_ptr[j] & ~(0xFFF));
				}
			}
		}
	}
	
	// register my interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	
}

void page_fault_interrupt_handler(registers regs)
{
	
	put_str("\nPage fault at virtual address ");
	put_hex(read_cr2());
	
	u32int present = regs.err_code & 0x1;
	__attribute__ ((unused)) u32int rw = regs.err_code & 0x2; // will be used later
	u32int us = regs.err_code & 0x4;
	
	if (!present)
	{
		// gather information
		u32int faulting_virt_addr = read_cr2();
		u32int page_dir_index = faulting_virt_addr >> 22;
		u32int page_table_index = (faulting_virt_addr >> 12) & 0x3FF;
		__attribute__ ((unused)) u32int page_offset = faulting_virt_addr & 0xFFF;
		
		// i should already have a pointer to the page directory
		// i want to rename it to make it easier to handle
		u32int *page_directory = current_page_directory->virt_addr;
		
		if ((page_directory[page_dir_index] & 0x1) == 0)
		{
			
			// create a pointer to the kernel's page table
			u32int *kernel_page_table = (u32int *) 0xFFF00000;
			
			// store the value on PT10 for later
			u32int PT10_tmp = kernel_page_table[10];
			
			// get a physical address for the new page table
			u32int new_table_phys_addr = alloc_frame();
			
			// map the new page to 0xC000A000
			kernel_page_table[10] = new_table_phys_addr | 3;
			
			// create a pointer to the new page so i can alter it
			u32int *new_page_table = (u32int *) 0xC000A000;
			
			// clear it
			memset((u8int *) new_page_table, 0, 4096);
			
			// create a page table in there
			for (u32int i = 0; i < 1024; i++)
			{
				new_page_table[i] = 0 | 2;
			}
			
			// put the physical address of the new page table on the page directory at the proper index, and set the attributes
			page_directory[page_dir_index] = (u32int) new_table_phys_addr | 3;
			
			// restore the value of the old mapping
			kernel_page_table[10] = PT10_tmp;
		}
		
		// get the physical address for the page table
		u32int table_phys_addr = page_directory[page_dir_index] & ~(0xFFF);
		
		// get the attributes for that page table
		u32int table_attribs = page_directory[page_dir_index] & 0xFFF;
		
		// create a pointer to the kernel's page table
		u32int *kernel_page_table = (u32int *) 0xFFF00000;
		
		// store the value on PT10 for later
		u32int PT10_tmp = kernel_page_table[10];
		
		// map the page table to 0xC000A000
		kernel_page_table[10] = table_phys_addr | table_attribs;
		
		// create a pointer to the page table so i can alter it
		u32int *table = (u32int *) 0xC000A000;
		
		// get a physical address for the new page
		u32int new_page_phys_addr = alloc_frame();
		
		// put the address on the page table w/ the proper attribues
		table[page_table_index] = new_page_phys_addr | table_attribs;
		
		// restore the value of the old mapping
		kernel_page_table[10] = PT10_tmp;
		
		// refresh the cr3 value
		write_cr3(read_cr3());
		
		// tell the TLB that the page table entry has been updated.
		invlpg((faulting_virt_addr & ~(0xFFF)));
		
		return;
		
	}
	else if (us)
	{
		u32int cr2_val = read_cr2();
		put_str("\nProtection fault. Memory at ");
		put_hex(cr2_val);
		put_str(" is reserved for supervisor.");
		put_str("\nError code: ");
		put_hex(regs.err_code);
		put_str("\nHalting system.");
		for (;;) {}
	}
	else
	{
		put_str("\nUnknown page fault exception has occured.");
		put_str("\nError code: ");
		put_hex(regs.err_code);
		put_str("\nHalting system.");
		for (;;) {}
	}
	
}

void invlpg(u32int addr)
{
	asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}

void set_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	frames[index] |= (0x1 << offset);
}

void clear_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	frames[index] &= (0x1 << offset);
}

u32int test_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	return (frames[index] & (0x1 << offset));
}

u32int first_free()
{
	for (u32int i = 0; i <= max_index; i++)
	{
		if (frames[i] != 0xFF)
		{
			for (u32int j = 0; j < 8; j++)
			{
				u32int test = (0x1 << j);
				if (!(frames[i] & test))
				{
					return ((i * 64) + j) * 0x1000;
				}
			}
		}
	}
	return 0xFFFFFFFF;
}

u32int alloc_frame()
{
	u32int phys_addr = first_free();
	
	if (phys_addr == 0xFFFFFFFF)
	{
		put_str("\nOut of physical memory.");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	set_frame(phys_addr);
	
	return phys_addr;
}

void switch_page_directory(__attribute__((unused)) page_directory_type *page_dir_ptr)
{
	
}

void map_page(u32int virt_addr, u32int phys_addr)
{
	
	// figure out the page directory index for the virtual address
	u32int page_dir_index = virt_addr >> 22;
	
	// figure out the page table index for the virtual address
	u32int page_table_index = (virt_addr >> 12) & 0x3FF;
	
	// figure out the recursive virtual address for the page table
	u32int virt_addr_base = 0xFFC00000;
	u32int virt_addr_offset = page_dir_index << 12;
	u32int page_table_virt_addr = virt_addr_base + virt_addr_offset;
	
	// make a pointer to the page table
	u32int *page_table = (u32int *) page_table_virt_addr;
	
	// if the frame for the physical address is allocated
	if (test_frame(phys_addr))
	{
		// then just map it
		// map the physical address to the virtual address by updating the entry on the page table
		page_table[page_table_index] = (phys_addr & ~(0xFFF)) | 3;
	}
	else
	{
		// allocate the frame for the physical address
		alloc_frame(phys_addr);
		
		// and map it
		// (mapping the address by writing to the page table should poke
		// the nonexisteant page table, causing a page fault, which
		// causes the page table to be created)
		page_table[page_table_index] = (phys_addr & ~(0xFFF)) | 3;
	}
	
	// tell the TLB that the page table entry has been updated.
	invlpg((virt_addr & ~(0xFFF)));
}

void unmap_page(u32int virt_addr)
{
	
	// figure out the page directory index for the virtual address
	u32int page_dir_index = virt_addr >> 22;
	
	// figure out the page table index for the virtual address
	u32int page_table_index = (virt_addr >> 12) & 0x3FF;
	
	// figure out the recursive virtual address for the page table
	u32int virt_addr_base = 0xFFC00000;
	u32int virt_addr_offset = page_dir_index << 12;
	u32int page_table_virt_addr = virt_addr_base + virt_addr_offset;
	
	// make a pointer to the page table
	u32int *page_table = (u32int *) page_table_virt_addr;
	
	// unmap the page
	page_table[page_table_index] = 0 | 2;
	
	// tell the TLB that the page table entry has been updated.
	invlpg((virt_addr & ~(0xFFF)));
	
}

void print_page_directory(volatile page_directory_type *page_directory)
{
	put_str("\n\tpage_directory=");
	put_hex((u32int) page_directory);
	
	put_str(" virt_addr=");
	put_hex((u32int) page_directory->virt_addr);
	
	put_str(" phys_addr=");
	put_hex(page_directory->phys_addr);
}




