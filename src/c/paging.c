#include <paging.h>

static void (*page_fault_handler)(u8int *buf, u16int size) = NULL;

u32int *page_directory;

void paging_initialize()
{
	// i need to set up a pointer to my page directory.
	// i can use the memory manager to allocate the first free page for the page directory address.
	*page_directory = allocate_block();
	
	// i need to zero the entries on the page directory
	for (int i = 0; i < 1024; i++)
	{
		//attribute: supervisor level, read/write, not present.
		page_directory[i] = 0 | 2;
	}
	
	// i need to create the first page table.
	// allocate another page for the page table
	u32int *page_table = (u32int *) allocate_block();
	
	// i need to fill in the page table, identity mapping (1:1) the first 4M
	u32int addr = 0;
	for (u32int i = 0; i < 1024; i++)
	{
		u32int page = addr & ~(0xFFF);
		// attributes: supervisor level, read/write, present.
		page_table[i] = page | 3;
		addr += 4096;
	}
	
	// i need to put the page table on the page directory
	// attributes: supervisor level, read/write, present
	page_directory[0] = (u32int) page_table | 3;
	
	// i need to register the page fault interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	// i need to load the page directory address into cr3
	write_cr3((u32int) page_directory);
	
	// i need to set the proper bit on cr0
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
}

void invlpg(u32int addr)
{
	asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}

void page_fault_set_handler(void (*callback)(u8int *buf, u16int size))
{
	page_fault_handler = callback;
}

void page_fault_interrupt_handler(registers regs)
{
	
	// close, but no cigar
	// something's wrong with this where it'll go alright until the
	// kernel level page fault handler is called
	// once it's called the system will lock
	// if it's not called, this function will enter an infinate loop
	// not sure what's causing this behavior.
	
	put_str("\nPage fault interrupt handler called.");
	
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
	
	u32int cr2_val = read_cr2();
	u32int cr3_val = read_cr3();
	u32int cr4_val = read_cr4();
	
	put_str("\ncr2 val is ");
	put_hex(cr2_val);
	
	put_str("\ncr3 val is ");
	put_hex(cr3_val);
	
	put_str("\ncr4 val is ");
	put_hex(cr4_val);
	
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
	
	// now what?
	
	if (!present) {
		// this is where i need to do the fancy stuff
		put_str("\nPage fault: Not present.");
		
		// does the page table that holds the page exist in the page directory?
		u32int page_table_index = cr2_val >> 22; // get the last 10 bits
		
		put_str("\nIndex of the desired page table is ");
		put_hex(page_table_index);
		
		// check if the page table for the desired page exists in the page directory
		
		put_str("\nValue on page directory at desired index is ");
		put_hex(page_directory[page_table_index]);
		
		// does a page table for that page exist?
		if (page_directory[page_table_index] == 0) { // this conditional is according to Yuri's code. not sure why this doesn't look at the first bit. doing so causes an infinate loop.
			put_str("\nPage table at desired index is not present.");
			
			// no, so create it
			u32int *page_table = (u32int *) allocate_block();
			put_str("\nMemory for new page allocated at ");
			put_hex((u32int) page_table);
			
			// clear the entries on the page table
			for (u32int i = 0; i < 1024; i++) {
				page_table[i] = 0; // if i'm getting an infinate looping when this function runs, this is probably causing it.
			}
			
			put_str("\nPage table created.");
			
			// update the page directory to reflect that the table is present.
			page_directory[page_table_index] = (u32int) page_table | 3;
			
			put_str("\nPage directory updated.");
		}
		
		// here i should request a physical page
		u32int page_to_map = allocate_block();
		put_str("\nPage to map is ");
		put_hex(page_to_map);
		
		// here i should map the page
		
		// i should get the physical address of the current page directory off the cr3 register
		// it's already on the cr3_val variable
		u32int *page_dir = (u32int *) cr3_val;
		put_str("\nPage directory: ");
		put_hex((u32int) page_dir);
		
		// i should calculate the index of the page table in the page directory
		u32int page_dir_index = page_to_map >> 22;
		put_str("\nPage directory index: ");
		put_hex(page_dir_index);
		
		// i should calculate the index of the page in the page table
		page_table_index = (page_to_map >> 12) & 0x3FF;
		put_str("\nPage table index: ");
		put_hex(page_table_index);
		
		// i should get the address of the page in the page table (last 20 bits)
		u32int *table_addr = (u32int *) (page_dir[page_dir_index] & ~(0xFFF));
		put_str("\nTable addr: ");
		put_hex((u32int) table_addr);
		
		// i should get the attributes of the page
		u32int page_attr = table_addr[page_table_index] & 0x03;
		put_str("\nPage attribs: ");
		put_hex(page_attr);
		
		// if there are no attributes then i should copy them from the page directory that the page table belongs to
		if (page_attr == 0) {
			u32int page_tbl_attr = page_dir[page_dir_index] & 0x03;
			page_attr = page_tbl_attr;
		}
		put_str(" is now: ");
		put_hex(page_attr);
		
		// i should update the page table's entry for the page
		table_addr[page_table_index] = page_to_map | page_attr;
		put_str("\nTable entry: ");
		put_hex(table_addr[page_table_index]);
		
		// here i should update the TLB w/ invlpg.
		invlpg(cr2_val);
		
	} else if (us) {
		// print out a message to the screen, and halt the system
		put_str("\nProtection fault. Memory at ");
		put_hex(cr2_val);
		put_str(" is reserved for supervisor.");
		put_str("\nError code: ");
		put_hex(regs.err_code);
		put_str("\nHalting system.");
		for (;;) {}
	} else {
		// something else happened, print out a message and halt the system
		put_str("\nUnknown page fault exception has occured.");
		put_str("\nError code: ");
		put_hex(regs.err_code);
		put_str("\nHalting system.");
		for (;;) {}
	}
	
	
	// do i really need to be calling a kernel level function?
	// might be handy in the future to keep the kernel aware of certain happenings.
	string msg = "\nPage Fault Interrupt Handler Done";
	page_fault_handler((u8int *) msg, strlen(msg));
	
	put_str("\nReturned from kernel level handler."); // works, then hangs
	
}













