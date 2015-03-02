#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

static u32int kernel_start __attribute__((unused)) = (u32int) &start; // unused for the moment, but i think i'll need it later
static u32int kernel_end = (u32int) &end;

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
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	
	// figure out how much that is in KB.
	u32int mem_in_kb = mem_in_mb * 1024;
	
	// figure out how much memory i have in bytes
	u32int mem_in_bytes = mem_in_kb * 1024;
	
	// figure out what the last physical memory address should be.
	u32int last_phys_mem_addr __attribute__((unused)) = mem_in_bytes - 1; // unused for the moment, but i think i'll need it later.
	
	// figure out how many page tables i'll need to create
	u32int tot_pages = mem_in_kb / 4;
	
	// figure out how many page tables i'll need
	u32int tot_page_tables = tot_pages / 1024;
	
	// figure out where i'll put my page directory.
	// i'll be putting it at the first page aligned address after the kernel.
	u32int page_dir_addr = kernel_end;
	page_dir_addr &= ~(0xFFF);
	page_dir_addr = page_dir_addr + 0x1000;
	
	// create a pointer to the place in memory where the page directory will actually be.
	u32int *page_directory;
	page_directory = (u32int *) page_dir_addr;
	
	// clear out the 4KB where the page directory will live (just to make sure there's no garbage 'cause ya never know)
	memset((u8int *) page_directory, 0, 4096);
	
	// create a blank page directory.
	// this will give me 1024 entries on the array that have the value of 0 | 2
	// this appears to be correct as per http://wiki.osdev.org/Setting_Up_Paging
	// (if i'm only making 32 page tables then why am I marking all of the others? future mappings?)
	for (int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2; // supervisor, read/write, not present
	}
	
	// figure out where i'm going to start putting my page tables.
	// page tables will be put right after the page directory, and sequential
	// since page tables are 4KB this will also serve as a counter.
	u32int page_table_addr = (u32int) &page_directory[1023] + 0x4;
	
	// i'll also need a counter to keep track of the physical memory addresses
	u32int phys_addr_counter = 0x0;
	
	// i need to loop tot_page_tables times to create all the page tables i'll need.
	// for each page table...
	for (u32int i = 0; i < tot_page_tables; i++)
	{
		// create a pointer to the place in memory where the page table will be
		u32int *page_table;
		page_table = (u32int *) page_table_addr;
		
		// clear out the 4KB of memory that'll hold the page table (to make sure there's no garbage in there).
		memset((u8int *) page_table, 0, 4096);
		
		// for each entry on the page table
		for (int j = 0; j < 1024; j++)
		{
			// put the proper physical memory address on the array
			page_table[j] = phys_addr_counter | 3; // supervisor, read/write, present
			
			// if the address is not inside the kernel, the page directory, or a page table, then it shouldn't be supervisor only. I'll change this later.
			
			// advance the counter
			phys_addr_counter += 0x1000;
		}
		
		// put the page table address in page_directory[i]
		page_directory[i] = (u32int) page_table | 3; // supervisor, read/write, present (as these correct for page directories that don't map to the kernel?)
		
		// increment the page table address counter.
		page_table_addr += 0x1000;
		
	}
	
	// i need to set the interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	// i need to put the address of the page directory on CR3
	write_cr3(page_dir_addr);
	
	// i need to set the paging enable bit on CR0
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
}

void page_fault_interrupt_handler(registers regs)
{
	put_str("\nPage Fault Interrupt Handler called.");
	
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
	
	u32int present = regs.err_code & 0x1;
	u32int rw = regs.err_code & 0x2;
	u32int us = regs.err_code & 0x4;
	u32int reserved = regs.err_code & 0x8;
	u32int id = regs.err_code & 0x10;
	
	put_str("\nError code evaluation:");
	if (present) put_str(" P");
	if (rw) put_str(" R/W");
	if (us) put_str(" U/S");
	if (reserved) put_str(" RSVD");
	if (id) put_str(" I/D");
	
	// everything above this line is for testing and debugging
	
	// evaluate the error code that's been pushed to the stack
	// cr2 register will hold the linear address that caused the exception on a page-not-present exception.
	// upper 10 bits will be the page directory entry, middle 10 bits will be the page table entry
	// check to see if the page directory entry present bit is set
	// if not, set up a page table, point the page directory entry to the base address of the page table, set the present bit, iretd
	// will need to map some physical memory to the page table, set the present bit, call invlpg, iretd
	
	// here's where I'll be doing the actual paging code.
	
	// everything below this line is for testing and debugging.
	
	put_str("\nEnd of page fault interrupt handler.");
}












