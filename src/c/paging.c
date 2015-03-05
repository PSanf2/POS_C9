#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

static u32int kernel_start = (u32int) &start;
static u32int kernel_end = (u32int) &end;

// static (visable only inside this file) variables i want to use to keep track of things as I go along.
static u32int mem_in_mb;
static u32int mem_in_kb;
static u32int mem_in_bytes;
static u32int last_phys_mem_addr;
static u32int tot_phys_pages;
static u32int first_page_after_kernel_phys_addr;
static u32int last_page_phys_addr;
static u32int stack_low_phys_addr;
static u32int stack_high_phys_addr;
static u32int stack_size_in_bytes;
static u32int first_page_after_stack;
static u32int page_dir_phys_addr;
static u32int page_table_phys_addr;

// static pointers i'll need to manage my stack of free pages
static u32int *page_stack_low;
static u32int *page_stack_ptr;
static u32int *page_stack_high;

// static pointers i'll need to enable paging
static u32int *page_directory;
static u32int *page_table;

void paging_print_info()
{
	// this is just going to be a general printing function to give me more information about the system
	put_str("\nPAGING INFORMATION");
	
	put_str("\nAvailable memory: ");
	put_dec(mem_in_mb);
	put_str(" MB = ");
	put_dec(mem_in_kb);
	put_str(" KB = ");
	put_dec(mem_in_bytes);
	put_str(" bytes");
	
	put_str("\nTotal pages: ");
	put_dec(tot_phys_pages);
	
	put_str("\nKernel start: ");
	put_hex(kernel_start);
	put_str(" Kernel end: ");
	put_hex(kernel_end);
	
	put_str("\nFirst page after kernel: ");
	put_hex(first_page_after_kernel_phys_addr);
	
	put_str("\nStack low: ");
	put_hex((u32int) page_stack_low);
	put_str(" Stack high: ");
	put_hex((u32int) page_stack_high);
	
	put_str("\nStack Pointer: ");
	put_hex((u32int) page_stack_ptr);
	put_str(" Size: ");
	put_dec(stack_size_in_bytes);
	put_str(" bytes");
	
	put_str("\nPage directory: ");
	put_hex((u32int) page_directory);
	put_str(" Page Table: ");
	put_hex((u32int) page_table);
	
	put_str("\nLast page table: ");
	put_hex(last_page_phys_addr);
	
	put_str("\nLast physical memory address: ");
	put_hex(last_phys_mem_addr);
	
	put_str("\n");
}

void paging_stack_initialize()
{
	// for the sake of sanity i'll be making some assumptions that'll cause the size of the stack to be larger than what's needed.
	// instead of figuring out the minimum size the stack will actually need to be i'll determine its size based on hypothetical maximums
		// that would be used if i was putting EVERY page aligned address from 0x0 to last_page_phys_addr on the stack.
	// this will make the stack larger than it actually needs to be, but will result in more sane programming.
	
	// this will have to be run before i enable paging.
	// when i initialize paging i'll include the stack in the identity mapping, and that's what will keep this working.
	
	// the stack will be put right after the kernel, so the low address will be the first page aligned address after the kernel
	stack_low_phys_addr = first_page_after_kernel_phys_addr;
	
	// the hypothetical maximum size of the stack is the total number of pages available to the system.
	// i'll need 4 bytes for each item on the stack
	stack_size_in_bytes = tot_phys_pages * 4;	
	
	// the high address will be where the stack begins.
	stack_high_phys_addr = stack_low_phys_addr + stack_size_in_bytes;
	
	// since i know i'll be using more space than the stack actually needs i'll bump the stack high address down four bytes
	// if i left it as is then the first item on the stack would actually be in another page
	// doing this saves me 4kb of space (a page)
	stack_high_phys_addr -= 0x4;
	
	// i need to put the addresses on the stack in reverse order from last_page_phys_addr to the first page aligned address on the stack.
	// i need to figure out what the first page aligned address after the stack is, and put that on the stack last.
	first_page_after_stack = stack_high_phys_addr;
	first_page_after_stack &= ~(0xFFF);
	first_page_after_stack += 0x1000;
	
	// create the pointers needed for the stack.
	page_stack_low = (u32int *) stack_low_phys_addr;
	page_stack_high = (u32int *) stack_high_phys_addr;
	page_stack_ptr = (u32int *) stack_high_phys_addr;
	
	// i need to populate the stack.
	// remember to put the last page address on the stack first, and work backwards.
	for (u32int i = last_page_phys_addr; i >= first_page_after_stack; i -= 0x1000)
	{
		paging_stack_push(i);
	}
	
}

void paging_stack_push(u32int phys_addr)
{
	// if the stack is full
	if (paging_stack_full())
	{
		// whine about it, and refuse to play any more
		put_str("\nPaging stack is full.");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	page_stack_ptr--;
	*page_stack_ptr = phys_addr;
}

u32int paging_stack_pop()
{
	// if the stack is empty
	if (paging_stack_empty())
	{
		put_str("\nPaging stack is empty.");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	u32int phys_addr = *page_stack_ptr;
	page_stack_ptr++;
	return phys_addr;
}

u32int paging_stack_full()
{
	return (page_stack_ptr == page_stack_low);
}

u32int paging_stack_empty()
{
	return (page_stack_ptr == page_stack_high);
}

void paging_initialize(struct multiboot *mboot_ptr)
{
	// make sure I have a memory map from the kernel.
	// if i don't have a memory map
	if (!(mboot_ptr->flags & 0x40))
	{
		// throw a fit, and refuse to play any more.
		put_str("\nGRUB failed to provide a memory map. Unable to initialize paging.");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	// gather information.
	
	// figure out how much memory i have in MB
	mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	
	// figure out how much that is in KB.
	mem_in_kb = mem_in_mb * 1024;
	
	// figure out how much memory i have in bytes
	mem_in_bytes = mem_in_kb * 1024;
	
	// figure out what the last physical memory address should be.
	last_phys_mem_addr = mem_in_bytes - 1;
	
	// figure out how many total pages of physical memory i have
	tot_phys_pages = mem_in_kb / 4;
	
	// figure out where my first free page aligned address is (where my free memory starts after the kernel)
	first_page_after_kernel_phys_addr = kernel_end;
	first_page_after_kernel_phys_addr &= ~(0xFFF);
	first_page_after_kernel_phys_addr += 0x1000;
	
	// figure out the physical address for my last page.
	last_page_phys_addr = last_phys_mem_addr;
	last_page_phys_addr &= ~(0xFFF);
	
	// get the stack of free pages set up.
	// this stack will start returning physical memory address that are located after the grub space, kernel, and stack space.
	// i'll be able to use the stack to get free page physical addresses.
	paging_stack_initialize();
	
	// i'm going to identity map the grub space, kernel, paging stack, one page directory, and one page table.
	// i'm not going to be mapping the entire 4MB that the page table can support. Just up to what's needed to map everthing i already have in memory.
	
	// figure out where i'll put my page directory
	page_dir_phys_addr = paging_stack_pop();
	
	// create a pointer to the page directory
	page_directory = (u32int *) page_dir_phys_addr;
	
	// clear out the 4KB where it will live.
	memset((u8int *) page_directory, 0, 4096);
	
	// create a blank page directory.
	for (int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2; // supervisor, read/write, not present
	}
	
	// figure out where my page table will be.
	page_table_phys_addr = paging_stack_pop();
	
	// create a pointer to the page table
	page_table = (u32int *) page_table_phys_addr;
	
	// clear out the space for the page table
	memset((u8int *) page_table, 0, 4096);
	
	// in order to keep this simple i'm going to use a physical address counter
	// this can be done with one less variable by using a hex and multiplication.
	// i only want to fill in entries on the page table for the pages that are actually in memory that need to be mapped.
	u32int phys_addr_counter = 0x0;
	for (int i = 0; i < 1024; i++)
	{
		// if the address i'm wanting to put on the page table is less than or equal than the address of the page table...
		// (if i mapped the first 4MB of memory then i'd be shooting myself in the foot because of the initial values on the stack)
		if (phys_addr_counter <= page_table_phys_addr)
		{
			page_table[i] = phys_addr_counter | 3; // supervisor, read/write, present.
		}
		else // i'm done.
		{
			break; // i could just pop more values off the stack to map the first 4MB at this point.
		}
		phys_addr_counter += 0x1000;
	}
	
	// put the page table on to the page directory.
	page_directory[0] = (u32int) page_table | 3; // supervisor, read/write, present
	
	// register my interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	// put the address of the page directory on CR3
	write_cr3((u32int) page_directory);
	
	// enable paging.
	write_cr0((u32int) (read_cr0() | 0x80000000));
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
			u32int new_table_phys_addr = paging_stack_pop();
			
			// map the new page to 0xA000
			page_table[10] = new_table_phys_addr | 3;
			
			// create a pointer to the new page
			u32int new_page_table_addr = 0xA000;
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
			
			//write_cr3(read_cr3()); // dunno why. yuri has this, but it doesn't seem to be needed.
			
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
		table = (u32int *) 0xA000;
		
		// get a physical address for the new page
		u32int new_page_phys_addr = paging_stack_pop();
		
		// put the address on the page table with the proper attributes
		table[page_table_index] = new_page_phys_addr | table_attribs;
		
		// restore the original mapping for 0xA000
		page_table[10] = PT10_temp;
		
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
