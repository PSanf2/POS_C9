#include <paging.h>
//#include <memory.h>

static void (*page_fault_handler)(u8int *buf, u16int size) = NULL;

void paging_initialize()
{
	u32int *page_directory = (u32int *) 0x9C000;
	u32int *page_table = (u32int *) 0x9D000;
	
	// filling the first page table
	u32int addr = 0x0;
	for (int i = 0; i < 1024; i++)
	{
		u32int page = addr & 0xFFFFF000;
		page_table[i] = page | 0x3;
		addr += 0x1000;
	}
	
	// putting the page table on the page directory
	page_directory[0] = (u32int) page_table | 0x3;
	
	// filling the rest of the page directory.
	for (int i = 1; i < 1024; i++)
	{
		page_directory[i] = 0;
	}
	
	// put the address of the page directory into the cr3 register
	write_cr3((u32int) page_directory);
	
	// set the paging bit on the cr0 register to enable paging
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
	// update the segment registers.
	update_segregs();
}

void page_fault_set_handler(void (*callback)(u8int *buf, u16int size))
{
	page_fault_handler = callback;
}

void page_fault_interrupt_handler(__attribute__ ((unused)) registers regs)
{
	string msg = "Page Fault";
	page_fault_handler((u8int *) msg, strlen(msg));
}













