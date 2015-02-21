#include <paging.h>

u32int *page_directory;
u32int *first_page_table;

void paging_initialize()
{
	// create a page directory
	page_directory = (u32int *) allocate_block();
	
	// clear the 4kb that the page directory occupies
	memset((u8int *) page_directory, 0, 4096);
	
	// create a page table
	first_page_table = (u32int *) allocate_block();
	
	// clear the 4kb that the page tables occupies
	memset((u8int *) first_page_table, 0, 4096);
	
	// blank the page directory
	for (int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2;	// attributes supervisor, read/write, not present
	}
	
	// put the page table on the page directory
	page_directory[0] = (u32int) first_page_table | 3;	// attributes supervisor, read/write, present
	
	// identity map the page table
	identity_map_page_table(page_directory, 0);
	
	// register the page fault interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	// load page directory into cr3 register
	write_cr3((u32int) page_directory);
	
	// set the paging enable bit on cr0
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
}

void identity_map_page_table(u32int *my_page_directory, u32int table_index_in_directory)
{
	// Thanks to Yuri
	u32int *table_pointer = (u32int *) my_page_directory[table_index_in_directory];
	table_pointer = (u32int *) ((u32int) table_pointer & ~(0xFFF));
	
	u32int page_phys_addr = table_index_in_directory * 4194304; // multiply by 4194304 for 4mb, already paged aligned by the pmm
	
	for (int i = 0; i < 1024; i++)
	{
		table_pointer[i] = page_phys_addr | 3;	// attributes supervisor, read/write, present
		page_phys_addr += 4096;
	}
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
