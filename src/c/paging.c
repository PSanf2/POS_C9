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

void print_paging_info()
{
	put_str("\nPage directory physical address is ");
	put_hex(page_dir_phys_addr);
	
	put_str("\nPage table physical address is ");
	put_hex(page_table_phys_addr);
	
}

void print_stack_info()
{
	put_str("\nStack low address is ");
	put_hex(stack_low_phys_addr);
	
	put_str("\nStack size in bytes is ");
	put_dec(stack_size_in_bytes);
	
	put_str("\nStack high address is ");
	put_hex(stack_high_phys_addr);
	
	put_str("\nLast address on the stack should be ");
	put_hex(first_page_after_stack);
	
}

void print_system_info()
{
	// this function is used to print out information
	put_str("\nSystem has ");
	put_dec(mem_in_mb);
	put_str(" MB of memory.");
	
	put_str("\nSystem has ");
	put_dec(mem_in_kb);
	put_str(" KB of memory.");
	
	put_str("\nSystem has ");
	put_dec(mem_in_bytes);
	put_str(" bytes of memory.");
	
	put_str("\nKernel starts at ");
	put_hex(kernel_start);
	
	put_str("\nKernel ends at ");
	put_hex(kernel_end);
	
	put_str("\nLast physical memory address is ");
	put_hex(last_phys_mem_addr);
	
	put_str("\nSystem has ");
	put_dec(tot_phys_pages);
	put_str(" pages of physical memory.");
	
	put_str("\nFirst page aligned physical address after kernel is ");
	put_hex(first_page_after_kernel_phys_addr);
	
	put_str("\nLast page aligned physical address is ");
	put_hex(last_page_phys_addr);
	
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
	
	//print_stack_info();
	
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
	//put_str("\nInitializing paging...");
	
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
	
	//print_system_info();
	
	// get the stack of free pages set up.
	// this stack will start returning physical memory address that are located after the grub space, kernel, and stack space.
	// i'll be able to use the stack to get free page addresses.
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
		// if the address i'm wanting to put on the page table is less than or equal the address of the page table...
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
	
	//print_paging_info();
	
	//put_str("\nDone initializing paging.\n");
}

void print_page_fault_info(registers regs)
{
	put_str("\nds=");
	put_hex(regs.ds);
	
	put_str("\nedi=");
	put_hex(regs.edi);
	
	put_str(" esi=");
	put_hex(regs.esi);
	
	put_str(" ebp=");
	put_hex(regs.ebp);
	
	put_str(" esp=");
	put_hex(regs.esp);
	
	put_str(" ebx=");
	put_hex(regs.ebx);
	
	put_str(" edx=");
	put_hex(regs.edx);
	
	put_str(" ecx=");
	put_hex(regs.ecx);
	
	put_str(" eax=");
	put_hex(regs.eax);
	
	put_str("\nint_no=");
	put_dec(regs.int_no);
	
	put_str(" err_code=");
	put_hex(regs.err_code);
	
	put_str("\neip=");
	put_hex(regs.eip);
	
	put_str(" cs=");
	put_hex(regs.cs);
	
	put_str(" eflags=");
	put_hex(regs.eflags);
	
	put_str(" useresp=");
	put_hex(regs.useresp);
	
	put_str(" ss=");
	put_hex(regs.ss);
	
	u32int cr0_val = read_cr0();
	u32int cr2_val = read_cr2();
	u32int cr3_val = read_cr3();
	
	put_str("\ncr0 val is ");
	put_hex(cr0_val);
	
	put_str("\ncr2 val is ");
	put_hex(cr2_val);
	
	put_str("\ncr3 val is ");
	put_hex(cr3_val);
}

void page_fault_interrupt_handler(registers regs)
{
	put_str("\nPage Fault Interrupt Handler called.");
	
	print_page_fault_info(regs);
	
	u32int present = regs.err_code & 0x1;
	u32int rw = regs.err_code & 0x2;
	u32int us = regs.err_code & 0x4;
	
	put_str("\nError code evaluation:");
	if (present) put_str(" P");
	if (rw) put_str(" R/W");
	if (us) put_str(" U/S");
	
	if (!present)
	{
		put_str("\nPage not present.");
		
		u32int faulting_virt_addr = read_cr2();
		
		put_str("\nFaulting address: ");
		put_hex(faulting_virt_addr);
		
		u32int page_dir_index = faulting_virt_addr >> 22; // get the first 10 bits from the faulting address
		
		put_str("\nPage directory index: ");
		put_dec(page_dir_index);
		
		u32int page_table_index = (faulting_virt_addr >> 12) & 0x3FF;
		
		put_str("\nPage table index: ");
		put_dec(page_table_index);
		
		u32int page_offset = faulting_virt_addr & 0xFFF;
		
		put_str("\nPage offset: ");
		put_dec(page_offset);
		
		// check to see if there's a page table at that index in the page directory
		// if not, i'll need to create a page table, and put it at that index
		
		put_str("\npage_directory[");
		put_dec(page_dir_index);
		put_str("] => ");
		put_hex(page_directory[page_dir_index]);
		
		// i'll be using virtual address 0xA000 (and maybe 0xB000) for tempoary mappings to create page directories and page tables.
		// remember to restore the original values when done!
		// save the mapping information for page_table[10]
		
		
		// if the present bit at page_directory[page_dir_index] == 0 then there is no page table
		if ((page_directory[page_dir_index] & 0x1) == 0)
		{
			put_str("\nPage table not found.");
			
			u32int PT10_temp = page_table[10];
		
			put_str("\nValue of page_table[10] is ");
			put_hex(page_table[10]);
			
			// get a physical address for the new page
			u32int new_table_phys_addr = paging_stack_pop();
			
			put_str("\nNew page table physical address will be ");
			put_hex(new_table_phys_addr);
			
			// map the new page to 0xA000
			page_table[10] = new_table_phys_addr | 3;
			
			// for testing
			put_str("\nValue of page_table[10] is ");
			put_hex(page_table[10]);
			
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
			
			// for testing
			put_str("\nValue of page_table[10] is ");
			put_hex(page_table[10]);
			
		}
		
		put_str("\npage_directory[");
		put_dec(page_dir_index);
		put_str("] => ");
		put_hex(page_directory[page_dir_index]);
		
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
		
		put_str("\nPhysical address for page table: ");
		put_hex(table_phys_addr);
		
		// get the attributes for that page table
		u32int table_attribs = page_directory[page_dir_index] & 0xFFF;
		
		put_str("\nPage table attributes: ");
		put_hex(table_attribs);
		
		// create a temp var to hold the original mapping of the virtual address i'll be using
		u32int PT10_temp = page_table[10];
		
		put_str("\nValue of page_table[10] is ");
		put_hex(page_table[10]);
		
		// map the page table to 0xA000
		page_table[10] = table_phys_addr | table_attribs;
		
		put_str("\nValue of page_table[10] is ");
		put_hex(page_table[10]);
		
		// create a pointer to the page table so i can alter it
		u32int *table;
		table = (u32int *) 0xA000;
		
		// testing the pointer and the values in the table
		put_str("\ntable[");
		put_dec(page_table_index);
		put_str("] => ");
		put_hex(table[page_table_index]);
		
		// get a physical address for the new page
		u32int new_page_phys_addr = paging_stack_pop();
		
		put_str("\nPhysical address for new page will be ");
		put_hex(new_page_phys_addr);
		
		// put the address on the page table with the proper attributes
		table[page_table_index] = new_page_phys_addr | table_attribs;
		
		// testing
		put_str("\ntable[");
		put_dec(page_table_index);
		put_str("] => ");
		put_hex(table[page_table_index]);
		
		// restore the original mapping for 0xA000
		page_table[10] = PT10_temp;
		
		put_str("\nValue of page_table[10] is ");
		put_hex(page_table[10]);
		
		invlpg((faulting_virt_addr & ~(0xFFF)));
		
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
	
	put_str("\nEnd of page fault interrupt handler.");
}

void invlpg(u32int addr)
{
	asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}












