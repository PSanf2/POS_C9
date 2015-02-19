#include <paging.h>

static void (*page_fault_handler)(u8int *buf, u16int size) = NULL;

u32int *page_directory;

void paging_initialize()
{
	// use the physical memory manager to allocate the first free 4K of memory for the page directory.
	page_directory = (u32int *) allocate_block();
	
	// zero the entries on the page directory
	for (int i = 0; i < 1024; i ++)
	{
		// attributes: read/write (i have a page directory, but no page tables, so none of them are present)
		page_directory[i] = 0 | 2;
	}
	
	// i need to create the first page table
	// use the physical memory manager to allocate another 4K of memory, clear it, put it on the page directory
	u32int *page_table = (u32int *) allocate_block();
	
	// fill the page table, identity mapping the first 4M of memory
	u32int addr = 0;
	for (int i = 0; i < 1024; i++)
	{
		// calculate the address of the page that the entry on the page table represents
		u32int page_addr = addr & ~(0xFFF); // sets the last 12 bits to 0
		// put the address of the page on the page table (attributes: supervisor level, read/write, present)
		page_table[i] = page_addr | 3;
		// update the addr var for the next value
		addr += 4096;
	}
	
	// put the page table on the page directory in the first spot on the array, and update the attributes
	// attributes: supervisor level, read/write, present in physical memory
	page_directory[0] = (u32int) page_table | 3;
	
	// register the page fault interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	// load the current page directory into the cr3 register
	write_cr3((u32int) page_directory);
	
	// set the proper bits on cr0 to enable paging
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
}

void page_fault_set_handler(void (*callback)(u8int *buf, u16int size))
{
	page_fault_handler = callback;
}

void page_fault_interrupt_handler(registers regs)
{
	put_str("\nPage Fault Interrupt Handler called.");
}













