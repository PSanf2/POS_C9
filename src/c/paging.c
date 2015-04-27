#include <paging.h>

extern bitmap_type *pmm_frames;
extern u32int kernel_start;
extern u32int kernel_end;

page_directory_type kernel_page_directory;
page_directory_type *current_page_directory;

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
	u32int page_table_virt_addr = page_dir_virt_addr + 0x1000;
	
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
	
	put_str("\nPage fault interrupt handler called.");
	
	
	u32int present = regs.err_code & 0x1;
	u32int rw = regs.err_code & 0x2;
	u32int us = regs.err_code & 0x4;
	
	if (!present)
	{
		// gather information
		u32int faulting_virt_addr = read_cr2();
		
		put_str("\nfaulting_virt_addr=");
		put_hex(faulting_virt_addr);
		
		u32int phys_addr = alloc_frame();
		
		put_str("\nphys_addr=");
		put_hex(phys_addr);
		
		map_page(faulting_virt_addr, phys_addr);
		
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

void map_page(u32int virt_addr, u32int phys_addr)
{
	// sanitise the inputs and make sure both of the addresses are page aligned.
	// examine the virtual address to determine the page dir index, and page table index
	// make sure there's a page table set up for that virtual address
	// if there isn't, create a page table, but make sure i'm not creating it at that physical address
	// update the page table to map the physical address
	// flush the TLB for the virtual address
	
	virt_addr &= ~(0xFFF); // sanitize the address, and make sure it's page aligned
	phys_addr &= ~(0xFFF);
	
	// figure out the page directory index, and page table index for the virtual address
	u32int page_dir_index = virt_addr >> 22;
	u32int page_table_index = (virt_addr >> 12) & 0x3FF;
	
	// create a pointer the page directory so i can work with it
	u32int *page_directory = current_page_directory->virt_addr;
	
	// if there's no page table for the frame
	if ((page_directory[page_dir_index] & 0x1) == 0)
	{
		// create a page table
		
		// allocate a frame for the new page table
		u32int table_phys_addr = alloc_frame();
		
		// make sure that's not the page we're trying to map
		if (table_phys_addr == phys_addr)
		{
			u32int temp = table_phys_addr;
			table_phys_addr = alloc_frame();
			free_frame(temp);
		}
		
		// create a pointer to the kernel's page table
		u32int *kernel_page_table = current_page_directory->tables[768].virt_addr;
		
		// store the value on PT10 for later
		u32int PT10_tmp = kernel_page_table[10];
		
		// map the new page to 0xC000A000
		kernel_page_table[10] = table_phys_addr | 3;
		
		// create a pointer ot the new page so i can alter it
		u32int *page_table = (u32int *) 0xC000A000;
		
		// clear it
		memset((u8int *) page_table, 0, 4096);
		
		// create a page table in there.
		for (u32int i = 0; i < 1024; i++)
		{
			page_table[i] = 0 | 2;
		}
		
		// put the physical address of the new page table on the page directory at the proper index
		page_directory[page_dir_index] = table_phys_addr | 3;
		
		// figure out the recursive address for the new page table
		u32int page_table_recursive_addr = 0xFFC00000 + (0x1000 * page_dir_index);
		
		// populate the proper values on the data structure
		current_page_directory->tables[page_dir_index].virt_addr = (u32int *) page_table_recursive_addr;
		current_page_directory->tables[page_dir_index].phys_addr = table_phys_addr;
		
		// restore the original mapping on PT10
		kernel_page_table[10] = PT10_tmp;
	}
	
	// get the attributes for the page table
	u32int table_attribs = get_table_attribs(page_dir_index);
	
	// create a pointer to the page table so i can alter it
	u32int *page_table = current_page_directory->tables[page_dir_index].virt_addr;
	
	// map the physical address
	page_table[page_table_index] = phys_addr | table_attribs;
	
	// flush the TLB for that page
	invlpg(virt_addr);
	
	return;
}

void unmap_page(u32int virt_addr)
{
	virt_addr &= ~(0xFFF); // sanitize the address, and make sure it's page aligned
	
	// figure out the page directory index, and page table index for the virtual address
	u32int page_dir_index = virt_addr >> 22;
	u32int page_table_index = (virt_addr >> 12) & 0x3FF;
	
	// create a pointer the page directory so i can work with it
	u32int *page_directory = current_page_directory->virt_addr;
	
	// if there's a page table for the frame
	if (page_directory[page_dir_index] & 0x1)
	{
		// create a pointer to the page table so i can alter it
		u32int *page_table = current_page_directory->tables[page_dir_index].virt_addr;
		
		// unmap the page
		page_table[page_table_index] = 0;
		
		// flush the TLB for that page
		invlpg(virt_addr);
	}
}

void change_page_directory(page_directory_type *page_directory)
{
	current_page_directory = page_directory;
	asm volatile(
		"mov %0, %%cr3"
		: /* no outputs */
		: "r" (page_directory->phys_addr)
	);
}

u32int get_table_attribs(u32int page_dir_index)
{
	return current_page_directory->virt_addr[page_dir_index] & 0xFFF;
}

void copy_page_directory(page_directory_type *source, page_directory_type *dest)
{
	for (u32int i = 0; i < 1024; i++)
	{
		dest->virt_addr[i] = source->virt_addr[i];
	}
}

void copy_page_table(page_table_type *source, page_table_type *dest)
{
	for (u32int i = 0; i < 1024; i++)
	{
		dest->virt_addr[i] = source->virt_addr[i];
	}
}
