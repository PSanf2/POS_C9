#include <paging.h>

static void (*page_fault_handler)(u8int *buf, u16int size) = NULL;

void paging_initialize()
{
	// i need to set up a pointer to my page directory.
	// i can use the memory manager to allocate the first free page for the page directory address.
	u32int *page_directory = (u32int *) allocate_block();
	
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

void page_fault_set_handler(void (*callback)(u8int *buf, u16int size))
{
	page_fault_handler = callback;
}

void page_fault_interrupt_handler(registers *regs)
{
	put_str("\nPage fault interrupt handler called.");
	put_str("\nError code is ");
	put_dec(regs->err_code); // this isn't working. why? i think something isn't being passed in properly.
	
	
	
	string msg = "\nPage Fault";
	page_fault_handler((u8int *) msg, strlen(msg));
}













