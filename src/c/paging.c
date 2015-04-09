#include <paging.h>

extern bitmap_type *pmm_frames;
extern u32int kernel_start;
extern u32int kernel_end;

page_directory_type kernel_page_directory;
page_directory_type *current_page_directory;

u32int page_table_virt_addr;

void paging_initialize()
{
	/*
	put_str("\nPaging initialize...");
	
	put_str("\nkernel_start=");
	put_hex(kernel_start);
	put_str(" kernel_end=");
	put_hex(kernel_end);
	*/
	
	// if the physical memory manager has not been initialized
	if ((u32int) pmm_frames == 0x0)
	{
		// throw a fit, and refuse to play any more.
		put_str("\nUnable to initialize paging.");
		put_str("\nPhysical memory manager has not been initialized.");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	/*
	 * When the kernel is loaded the boot script gets things setup with
	 * 4 MB paging. The system has one page directory set up, and the
	 * first thing that happens is that the physical memory manager gets
	 * set up. The system will have 4 MB of memory mapped at this point.
	 * The first thing I want to do is replace the 4 MB page
	 * table with a 4 KB page table. In order to do this I'll need to
	 * create a new page directory, and new page table. Each of these
	 * tasks will require 4 KB of space to be mapped ahead of time.
	 * I'll need to make sure that I don't clobber the PMM bitmap, and
	 * decide where these new paging structures will live in memory.
	 * I'll also need to make sure that these addresses are within the
	 * 4 MB of space that was mapped when the kernel was loaded.
	 */

	// figure out where I'd like to put the page directory
	u32int page_dir_virt_addr = (u32int) pmm_frames->addr + pmm_frames->bytes;

	// make sure that address is page aligned.
	if (page_dir_virt_addr % 0x1000 != 0)
	{
		page_dir_virt_addr = (page_dir_virt_addr & ~(0xFFF)) + 0x1000;
	}

	// figure out where i'd like to put the page table
	page_table_virt_addr = page_dir_virt_addr + 0x1000;
	
	/*
	put_str("\npage_dir_virt_addr=");
	put_hex(page_dir_virt_addr);
	put_str("\tpage_table_virt_addr=");
	put_hex(page_table_virt_addr);
	*/

	// make sure both of these are in the mapped 4MB of memory
	// if the page directory address, or page table address, will put any part of the structure outside the mapped memory
	if ((page_dir_virt_addr > 0xC03FE000) || (page_table_virt_addr >0xC03FE000))
	{
		// throw a coniption and refuse to play any more.
		put_str("\nUnable to initialize paging.");
		put_str("\nPage directory virtual address, or page table virtual address, is too high.");
		put_str("\npage_dir_virt_addr=");
		put_hex(page_dir_virt_addr);
		put_str("\npage_table_virt_addr=");
		put_hex(page_table_virt_addr);
	}

	/*
	* I need to figure out the physical addresses that I'll use for my
	* new page directory, and page table. This is the only place in
	* the whole system where this trick will ever work.
	*/
	
	u32int page_dir_phys_addr = page_dir_virt_addr - 0xC0000000;
	u32int page_table_phys_addr = page_table_virt_addr - 0xC0000000;
	
	/*
	put_str("\npage_dir_phys_addr=");
	put_hex(page_dir_phys_addr);
	put_str("\tpage_table_phys_addr=");
	put_hex(page_table_phys_addr);
	put_str("\n");
	*/
	
	// i need to make a pointer to the place where the page directory will be
	u32int *page_dir_ptr = (u32int *) page_dir_virt_addr;
	
	// i need to clear out that 4 KB of space.
	memset((u8int *) page_dir_ptr, 0, 4096);
	
	// i need to create an empty page directory in there
	for (u32int i = 0; i < 1024; i++)
	{
		page_dir_ptr[i] = 0 | 2;
	}
	
	// i need to make a pointer to the place where the page table will be
	u32int *page_table_ptr = (u32int *) page_table_virt_addr;
	
	// i need to clear out that 4 KB of space
	memset((u8int *) page_table_ptr, 0, 4096);
	
	for (u32int i = 0; i < 1024; i++)
	{
		page_table_ptr[i] = (i * 0x1000) | 3;
	}
	
	// i need to figure out the index for the kernel page directory entry
	u32int kernel_index = 0xC0000000 >> 22;
	
	// i need to put the physical address of the page table on the page directory
	page_dir_ptr[kernel_index] = page_table_phys_addr | 3;
	
	// i need to set up the recursive mappings on the page directory.
	page_dir_ptr[1023] = page_dir_phys_addr | 3;
	
	// save the virtual and physical address of the new page directory
	kernel_page_directory.virt_addr = (u32int *) page_dir_virt_addr;
	kernel_page_directory.phys_addr = page_dir_phys_addr;
	
	// empty out the "page tables" on the page directory data structure
	for (u32int i = 0; i < 1024; i++)
	{
		kernel_page_directory.tables[i].virt_addr = 0;
		kernel_page_directory.tables[i].phys_addr = 0;
	}
	
	// put the stuff for the kernel page table on the data structures
	kernel_page_directory.tables[kernel_index].virt_addr = (u32int *) 0xFFF00000;
	kernel_page_directory.tables[kernel_index].phys_addr = page_table_phys_addr;
	
	kernel_page_directory.tables[1023].virt_addr = (u32int *) 0xFFC00000;
	kernel_page_directory.tables[1023].phys_addr = page_dir_phys_addr;
	
	// set the kernel page directory as the current page directory
	current_page_directory = (page_directory_type *) &kernel_page_directory;
	
	// i need to figure out what to put on CR4 to switch the system to 4 KB pages
	u32int new_cr4_val = read_cr4() & ~(0x00000010);
	
	// time to do the magic
	// switch the system over to 4KB paging, and give it the phys addr of the new page dir
	asm volatile (
		"mov %0, %%cr3\n\t"
		"mov %1, %%cr4"
		: /* no outputs */
		: "r" (page_dir_phys_addr), "r" (new_cr4_val)
	);
	// say a prayer.
	
	// register my interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	//put_str("\nDone.\n");
}

void page_fault_interrupt_handler(registers regs)
{
	//put_str("\nPage fault interrupt handler called.");
	
	u32int present = regs.err_code & 0x1;
	u32int rw = regs.err_code & 0x2;
	u32int us = regs.err_code & 0x4;
	
	if (!present)
	{
		// gather information
		u32int faulting_virt_addr = read_cr2();
		u32int page_dir_index = faulting_virt_addr >> 22;
		u32int page_table_index = (faulting_virt_addr >> 12) & 0x3FF;
		//u32int page_offset = faulting_virt_addr & 0xFFF;
		
		// make a pointer to the current page directory's virtual address
		u32int *page_directory = current_page_directory->virt_addr;
		
		/*
		// print out some information for debugging
		put_str("\nfaulting_virt_addr=");
		put_hex(faulting_virt_addr);
		put_str("\npage_dir_index=");
		put_dec(page_dir_index);
		put_str("\npage_table_index=");
		put_dec(page_table_index);
		put_str("\npage_offset=");
		put_hex(page_offset);
		put_str("\npage_directory=");
		put_hex((u32int) page_directory);
		*/
		
		// if the page table for the faulting address is not present
		if ((page_directory[page_dir_index] & 0x1) == 0x0)
		{
			
			// i need to allocate 4KB of page aligned virtual address
			// space above the end of the kernel
			//u32int *table_virt_addr = malloc_above(0x1000, 0x1000, kernel_end); <--- NO!
			// i should be figuring out the recursively mapped virtual address that the page table will be using.
			// before i'll be able to write to the page table i'll need to create the mappings for it?
			// FFC0 0000 = page table 0 virt addr
			// FFC0 1000 = page table 1
			// FFC0 2000 = page table 2
			// etc...
			// FFFF F000 = page table 1023
			// the page dir index should tell me which page table i'll want to write to.
			// i believe i can update the information on the data structure,
			// update the data in the real page directory, and flush the tlb, and i'll be able to write to the page table.
			// figuring out the address for the page table should be FFC0 0000 + 0x1000 * page dir index
			// The above won't help me when creating a new page table, just for keeping track of them.
			// And it's the value i'll need to put on the data structure
			// bottom line is that this should be very similar to the previous version, with the addition of a few lines to
			// keep the stuff on the data structure up to date.
			
			// i need to make a pointer to the kernel's page table
			u32int *kernel_page_table = current_page_directory->tables[768].virt_addr;
			
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
			
			// i need to figure out the recursive address where the new page table lives
			u32int page_table_recursive_addr = 0xFFC00000 + (0x1000 * page_dir_index);
			
			// populate the proper values on the data structure
			current_page_directory->tables[page_dir_index].virt_addr = (u32int *) page_table_recursive_addr;
			current_page_directory->tables[page_dir_index].phys_addr = new_table_phys_addr;
			
			// restore the value of the old mapping
			kernel_page_table[10] = PT10_tmp;
			
		}
		
		// get the attributes for that page table
		u32int table_attribs = get_table_attribs(page_dir_index);
		
		// create a pointer to the page table so i can alter it
		u32int *table = current_page_directory->tables[page_dir_index].virt_addr;
		
		// get a physical address for the new page
		u32int new_page_phys_addr = alloc_frame();
		
		// put the address on the page table w/ the proper attribues
		table[page_table_index] = new_page_phys_addr | table_attribs;
		
		// refresh the cr3 value
		write_cr3(read_cr3());
		
		// tell the TLB that the page table entry has been updated.
		invlpg((faulting_virt_addr & ~(0xFFF)));
		
		return;
		
	}
	else if (rw)
	{
		u32int cr2_val = read_cr2();
		put_str("\nWrite fault. Memory at ");
		put_hex(cr2_val);
		put_str(" is read only.");
		put_str("\nError code: ");
		put_hex(regs.err_code);
		put_str("\nHalting system.");
		for (;;) {}
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
	
	/*
	put_str("\nPage fault handler done.");
	put_str("\nHalting.");
	for(;;) {}
	*/
}

void invlpg(u32int addr)
{
	asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}

u32int phys_to_virt(page_directory_type *page_directory, u32int phys_addr)
{
	u32int result = 0xFFFFFFFF;
	u32int look_for = phys_addr & ~(0xFFF);
	u32int offset = phys_addr & 0xFFF;
	boolean found = FALSE;
	u32int *page_dir_ptr = page_directory->virt_addr;
	
	// for each entry on the page directory
	for (u32int i = 0; i < 1024 && found == FALSE; i++)
	{
		// if there's a page table present
		if (page_dir_ptr[i] & 0x1)
		{
			// make a pointer to that page table
			u32int *page_table_ptr = page_directory->tables[i].virt_addr;

			// for each entry on the page table
			for (u32int j = 0; j < 1024 && found == FALSE; j++)
			{
				// if that page table entry is what i'm looking for
				if ((page_table_ptr[j] & ~(0xFFF)) == look_for)
				{
					// calculate the result
					result = (i * 0x400000) + (j * 0x1000) + offset;
					// and stop looping
					found = TRUE;
				}
			}
		}
	}
	
	return result;
}

u32int virt_to_phys(page_directory_type *page_directory, u32int virt_addr)
{
	u32int result = 0xFFFFFFFF;
	
	// calculate the page directory index, table index, and table offset
	u32int page_dir_index = virt_addr >> 22;
	u32int page_table_index = (virt_addr >> 12) & 0x3FF;
	u32int table_offset = virt_addr & 0xFFF;
	
	// make a pointer to the page directory
	u32int *page_dir_ptr = page_directory->virt_addr;
	
	// if there's something present at that page directory index
	if (page_dir_ptr[page_dir_index] & 0x1)
	{
		// make a pointer to the page table
		u32int *page_table_ptr = page_directory->tables[page_dir_index].virt_addr;
		
		if (page_dir_index == 1023)
		{
			page_table_ptr = page_dir_ptr;
		}
		
		// if the page table entry for the physical address i'm curious about is present
		if (page_table_ptr[page_table_index] & 0x1)
		{
			// calculate the result
			result = (page_table_ptr[page_table_index] & ~(0xFFF)) + table_offset;
		}
	}
	
	return result;
}

void map_page(page_directory_type *page_directory, u32int virt_addr, u32int phys_addr)
{
	put_str("\npage_directory=");
	put_hex((u32int) page_directory);
	put_str("\npage_directory->virt_addr=");
	put_hex((u32int) page_directory->virt_addr);
	put_str(" page_directory->phys_addr=");
	put_hex(page_directory->phys_addr);
	
	put_str("\nvirt_addr=");
	put_hex(virt_addr);
	put_str("\tphys_addr");
	put_hex(phys_addr);
	
	/*
	 * With this function I will be looking to map a physical frame to
	 * a virtual page. I will need to make sure I update both the the
	 * real page directory stuff, and the data structures. If I discover
	 * that I need to create a new page table before mapping the desired
	 * addresses then I should make sure that I don't attempt to create
	 * a new page table at the specified physical address. This would be
	 * bad because writing to the virtual address would then clobber the
	 * newly created page table, invalidate the information on the page
	 * directory data structure, and ultimatly crash the system. If I
	 * discover that the next free frame is the one for the physical
	 * address in question, then I should allocate the next one, create
	 * the page table, and free the previous frame.
	 */

	// page align the virtual and physical address
	phys_addr &= ~(0xFFF);
	virt_addr &= ~(0xFFF);
	
	put_str("\nvirt_addr=");
	put_hex(virt_addr);
	put_str("\tphys_addr");
	put_hex(phys_addr);
	
	// figure out the page directory index, and page table index for
	// the virtual address that's been provided.
	u32int page_dir_index = virt_addr >> 22;
	u32int page_table_index = (virt_addr >> 12) & 0x3FF;
	
	put_str("\npage_dir_index=");
	put_dec(page_dir_index);
	put_str("\npage_table_index=");
	put_dec(page_table_index);
	
	// make a pointer to the page directory
	u32int *page_dir_ptr = page_directory->virt_addr;
	
	// check to see if I already have a page table. if I don't, create one
	// if there isn't a page table on the index.
	if ((page_dir_ptr[page_dir_index] & 0x1) == 0)
	{
		// allocate a frame for the new page table
		u32int table_frame = alloc_frame();
		
		// if that frame is for the physical address i'm trying to map
		if (phys_addr == table_frame)
		{
			u32int temp = table_frame;
			table_frame = alloc_frame();
			free_frame(temp);
		}
		
		// i need to allocate 4 KB of page aligned virtual memory, and
		// make sure it isn't the virtual address i'm trying to work with.
		// i need to get the VMM working before I can do this.
		// oops.
		// got that done. I can continue on this.
		
	}
	
	put_str("\n");
}

u32int table_attribs(u32int page_dir_index)
{
	u32int *page_directory = current_page_directory->virt_addr;
	return page_directory[page_dir_index] & 0xFFF;
}

/*
void unmap_page(page_directory_type *page_directory, u32int virt_addr)
{
	
}
*/

/*
void change_page_directory(page_directory_type *page_directory)
{
	
}
*/

u32int get_table_attribs(u32int page_dir_index)
{
	u32int *page_directory = current_page_directory->virt_addr;
	return page_directory[page_dir_index] & 0xFFF;
}
