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
		page_directory[i] = 0 | 2;
	}
	
	// put the page table on the page directory
	page_directory[0] = (u32int) first_page_table | 3;
	
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
		table_pointer[i] = page_phys_addr | 3;
		page_phys_addr += 4096;
	}
}

void page_fault_interrupt_handler(registers regs)
{
	put_str("\nPage Fault Interrupt Handler called.");
}
