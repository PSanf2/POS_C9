#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

__attribute__((unused)) u32int kernel_start = (u32int) &start;
u32int kernel_end = (u32int) &end;

u32int *page_directory;
static u32int *page_table;

static u8int *bitmap;
static u32int max_index;

void paging_initialize(struct multiboot *mboot_ptr)
{
	
	// gather information.
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	u32int mem_in_kb = mem_in_mb * 1024;
	u32int tot_4kb_pages = mem_in_kb / 4;
	u32int page_directory_phys_addr = read_cr3();
	u32int page_directory_virt_addr = page_directory_phys_addr + 0xC0000000;
	
	// figure out the address that i want to use for my new page table.
	u32int new_page_table_virt_addr = kernel_end;
	new_page_table_virt_addr &= ~(0xFFF);
	new_page_table_virt_addr += 0x1000;
	
	// figure out the physical address for the new page table.
	u32int new_page_table_phys_addr = new_page_table_virt_addr - 0xC0000000;
	
	// get an address that i want to use for my temp page directory
	u32int temp_page_dir_virt_addr = new_page_table_virt_addr + 0x1000;
	
	// figure out the physical address for that temp page directory.
	u32int temp_page_dir_phys_addr = temp_page_dir_virt_addr - 0xC0000000;
	
	// make a pointer to the temp page directory.
	u32int *temp_page_directory = (u32int *) temp_page_dir_virt_addr;
	
	// clear out 4kb of space for the page directory.
	memset((u8int *) temp_page_directory, 0, 4096);
	
	// make a page directory in there
	for (int i = 0; i < 1024; i++)
	{
		temp_page_directory[i] = 0 | 2;
	}
	
	// create a pointer to the new page table
	page_table = (u32int *) new_page_table_virt_addr;
	
	// clear out the 4kb of space needed for the page table.
	memset((u8int *) page_table, 0, 4096);
	
	// this area is going to start causing problems once the kernel gets
	// above 4MB in size. i won't be mapping the crap beyond the initial
	// 4MB, and i'll start getting page faults.
	// i'll need to make sure i keep all of the system vital stuff
	// below 4MB until i can start mapping other stuff in.
	
	// create a page table for the new mappings.
	u32int phys_addr_ctr = 0x0;
	for (int i = 0; i < 1024; i++)
	{
		if (phys_addr_ctr <= temp_page_dir_phys_addr)
		{
			page_table[i] = phys_addr_ctr | 3; // attributes supervisor, read/write, present
		}
		else
		{
			page_table[i] = 0 | 2;
		}
		phys_addr_ctr += 0x1000;
	}
	
	// the kernel is being mapped to 0xC0000000 so i need to figure out the proper index on the page directory for that address.
	u32int kernel_page_dir_index = 0xC0000000 >> 22;
	
	// put the physical address of the page table on the page directory
	temp_page_directory[kernel_page_dir_index] = new_page_table_phys_addr | 3; // attributes supervisor, read/write, present
	
	// get the cr4 value
	u32int new_cr4_val = read_cr4();
	
	// figure out what the new value should be
	new_cr4_val &= ~(0x00000010);
	
	// i'll need to use an asm volatile statement here to move the new cr4 value, and the new page directory physical address into the control registers.
	// i can't call any c functions to do this because doing so causes the whole system to triple fault as soon as either value is changed.
	asm volatile (
		"mov %0, %%cr3\n\t"
		"mov %1, %%cr4"
		: /* no outputs */
		: "r" (temp_page_dir_phys_addr), "r" (new_cr4_val)
	);
	
	// enable paging (just to make sure i've done it)
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
	// i now need to reclaim the space used for the original page directory, and
	// set my actual page directory up in the proper place.
	page_directory = (u32int *) page_directory_virt_addr;
	
	// clear out the space
	memset((u8int *) page_directory, 0, 4096);
	
	// create  a page table in there
	for (int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2;
	}
	
	// put the physical address of the page table on the page directory
	page_directory[kernel_page_dir_index] = new_page_table_phys_addr | 3; // attributes supervisor, read/write, present
	
	// i now need to set up the stuff i'll need for the recursive mapping.
	// i'll want to put the physical address of the page directory on
	// to page_directory[1023]. If I do this right then i should be able
	// to access my page tables through 0xFFC00000 - 0xFFFFFFFF.
	// the first time i try to access any of these addresses i'll end
	// up causing a page fault.
	
	// FFC0 0000 = page table 0 virt addr
	// FFC0 1000 = page table 1
	// FFC0 2000 = page table 2
	// etc...
	// FFFF F000 = page table 1023
	
	// FFF0 0000 should be the virtual address for the kernel page table.
	
	page_directory[1023] = page_directory_phys_addr | 3; // supervisor, read/write, present
	
	// tell the system to use the new page directory
	write_cr3(page_directory_phys_addr);
	
	// i now need to set up my bitmap.
	// i'll need to figure out how large the bitmap will need to be in
	// bytes, then kilobytes, then pages.
	// i'll then need to map in the number of pages needed for the bitmap.
	// i'll then need to make a pointer to the bitmap, initialize it to
	// all zeros, then go over the page directory, and page tables to 
	// figure out which frames are used.
	
	// i should need a theoretical maximum of 1MB of space for the bitmap
	// which would be a maximum of 32 page tables.
	// the tot_4kb_pages var tells me how many page frames i have
	// for each 4kb frame i'll need 1 bit.
	
	u32int bytes_needed_for_bitmap = tot_4kb_pages / 8;
	
	u32int frames_needed_for_bitmap = bytes_needed_for_bitmap / 0x1000;
	
	if (bytes_needed_for_bitmap % 0x1000 != 0)
	{
		frames_needed_for_bitmap++;
	}
	
	// in order to keep things simple i'm going to put my
	// bitmap in the virtual address range right below what
	// i'm using for my page tables.
	// the virtual address range for the bitmap will be from
	// 0xFFB00000 - 0xFFBFFFFF
	// they'll be in page_directory[1022]
	
	// i need to find a physical address for the first page table
	// i should be able to recycle/reuse the physical address for
	// the temp page directory for a new page table
	
	//page_directory[1022] = temp_page_dir_phys_addr | 3; yes, but only after i setup the page table for the bitmap.
	// that's where the page table for the bitmap will need to go
	
	// it's already there from a previous mapping. i just need to clear it out
	// and reuse it as a page table.
	
	// get the virtual address for the bitmap page table
	u32int bitmap_page_table_virt_addr = temp_page_dir_virt_addr;
	
	// get the physical address for the bitmap page table
	u32int bitmap_page_table_phys_addr = temp_page_dir_phys_addr;
	
	// make a pointer to it
	u32int *bitmap_page_table;
	bitmap_page_table = (u32int *) bitmap_page_table_virt_addr;
	
	// clear out that section of memory.
	memset((u8int *) bitmap_page_table, 0, 4096);
	
	// create a page table in there
	for (int i = 0; i < 1024; i++)
	{
		bitmap_page_table[i] = 0 | 2;
	}
	
	// figure out the physical address where the bitmap will start
	u32int bitmap_phys_addr = bitmap_page_table_phys_addr + 0x1000;
	
	// put the required pages on the bitmap page table
	u32int bitmap_phys_addr_ctr = bitmap_phys_addr;
	for (u32int i = 768; i < (768 + frames_needed_for_bitmap); i++)
	{
		bitmap_page_table[i] = bitmap_phys_addr_ctr | 3;
	}
	
	// put the address of the bitmap page table on the page directory at the desired index
	page_directory[1022] = bitmap_page_table_phys_addr | 3;
	
	// make a pointer for the bitmap.
	bitmap = (u8int *) 0xFFB00000;
	
	// figure out the max index of the bitmap array
	max_index = (tot_4kb_pages / 64) - 1;
	
	// clear the bitmap
	for (u32int i = 0; i < tot_4kb_pages; i++)
	{
		clear_frame(i * 0x1000);
	}
	
	// i now need to go over the page directory, and page tables to see
	// which physical frames are being used.
	
	// in order to check the page tables i'll have to create a pointer
	// to the proper virtual memory address based off the PD index
	
	// for each page directory entry
	for (u32int i = 0; i < 1024; i++)
	{
		// if there's a page table present
		if (page_directory[i] & 0x1)
		{
			set_frame(page_directory[i] & ~(0xFFF));
			// now i need to figure out the virtual address where the
			// page table will be recursivly mapped.
			u32int virt_addr_base = 0xFFC00000;
			u32int virt_addr_offset = i << 12;
			u32int page_table_virt_addr = virt_addr_base + virt_addr_offset;
			// make a pointer to that virtual address
			u32int *page_table_ptr;
			page_table_ptr = (u32int *) page_table_virt_addr;
			
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
	
	// get the virtual memory manager going
	// this address should get the VMM bitmap located right below
	// the address range used for the physical frames bitmap.
	vmm_initialize(0xFFA00000);
	
}

void set_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	bitmap[index] |= (0x1 << offset);
}

void clear_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	bitmap[index] &= (0x1 << offset);
}

u32int test_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	return (bitmap[index] & (0x1 << offset));
}

u32int first_free()
{
	for (u32int i = 0; i <= max_index; i++)
	{
		if (bitmap[i] != 0xFF)
		{
			for (u32int j = 0; j < 8; j++)
			{
				u32int test = (0x1 << j);
				if (!(bitmap[i] & test))
				{
					return ((i * 64) + j) * 0x1000;
				}
			}
		}
	}
	return 0xFFFFFFFF;
}

void page_fault_interrupt_handler(registers regs)
{
	
	u32int present = regs.err_code & 0x1;
	__attribute__ ((unused)) u32int rw = regs.err_code & 0x2; // not used now, but may be used later.
	u32int us = regs.err_code & 0x4;
	
	if (!present)
	{
		
		u32int faulting_virt_addr = read_cr2();
		
		u32int page_dir_index = faulting_virt_addr >> 22; // get the first 10 bits from the faulting address
		
		u32int page_table_index = (faulting_virt_addr >> 12) & 0x3FF;
		
		__attribute__ ((unused)) u32int page_offset = faulting_virt_addr & 0xFFF;
		
		// check to see if there's a page table at that index in the page directory
		// if not, i'll need to create a page table, and put it at that index
		
		// i'll be using virtual address 0xA000 for tempoary mappings to create new page directories and new page tables.
		// remember to restore the original values when done!
		// save the mapping information for page_table[10]
		
		// if the present bit at page_directory[page_dir_index] == 0 then there is no page table
		if ((page_directory[page_dir_index] & 0x1) == 0)
		{
			
			u32int PT10_temp = page_table[10];
			
			// get a physical address for the new page
			u32int new_table_phys_addr = first_free();
			
			if (new_table_phys_addr == 0xFFFFFFFF)
			{
				// this is where i will take care of things like swapping.
				put_str("\nOut of physical memory.");
				put_str("\nHalting.");
				for (;;) {}
			}
			
			
			set_frame(new_table_phys_addr);
			
			// map the new page to 0xA000
			page_table[10] = new_table_phys_addr | 3;
			
			// create a pointer to the new page
			u32int new_page_table_addr = 0xC000A000;
			u32int *new_page_table;
			new_page_table = (u32int *) new_page_table_addr;
			
			// clear it
			memset((u8int *) new_page_table, 0, 4096);
			
			// create the page table
			for (int i = 0; i < 1024; i++)
			{
				new_page_table[i] = 0 | 2; // supervisor, read/write, not present
			}
			
			// put the physical address of the new page table on the page directory at the proper index, and set the attributes
			page_directory[page_dir_index] = (u32int) new_table_phys_addr | 3; // supervisor, read/write, present.
			
			// restore the value of the old mapping
			page_table[10] = PT10_temp;
			
		}
		
		// i now need to check if the page is present on the page table, and map a new page if needed
		// i already know that the page is not present because that's what caused the page fault
		// i need to get a physical address for a new page
		// map the page table to somewhere so i can manipulate it
		// put the physical address of the new page on the page table at the proper index
		// and set the attributes on the page table
		// restore the original mapping
		
		// i don't think i should zero the new page. if there's garbage in there, then oh well.
		// i don't want to clear it because it may have been previously written to, and have infomration that something does want to read.
		
		// get the physical address of the page table
		u32int table_phys_addr = page_directory[page_dir_index] & ~(0xFFF);
		
		// get the attributes for that page table
		u32int table_attribs = page_directory[page_dir_index] & 0xFFF;
		
		// create a temp var to hold the original mapping of the virtual address i'll be using
		u32int PT10_temp = page_table[10];
		
		// map the page table to 0xA000
		page_table[10] = table_phys_addr | table_attribs;
		
		// create a pointer to the page table so i can alter it
		u32int *table;
		table = (u32int *) 0xC000A000;
		
		// get a physical address for the new page
		u32int new_page_phys_addr = first_free();
		
		if (new_page_phys_addr == 0xFFFFFFFF)
		{
			// this is where i'll need to take care of things like swapping.
			put_str("\nOut of physical memory.");
			put_str("\nHalting.");
			for (;;) {}
		}
		
		set_frame(new_page_phys_addr);
		
		// put the address on the page table with the proper attributes
		table[page_table_index] = new_page_phys_addr | table_attribs;
		
		// restore the original mapping for 0xA000
		page_table[10] = PT10_temp;
		
		write_cr3(read_cr3()); // dunno why. yuri has this, and it seems to be needed.
		// it's needed because of the way i'm doing temp mappings in order to create new page tables.
		
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
