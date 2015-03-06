#include <paging.h>

void paging_print_info()
{
	// this is just going to be a general printing function to give me more information about the system
	put_str("\nPAGING INFORMATION");
	
}

void paging_initialize(struct multiboot *mboot_ptr)
{
	put_str("\nPAGING INITIALIZATION FUNCTION CALLED.");
	for (;;) {}
}

void page_fault_interrupt_handler(registers regs)
{
	put_str("\nPAGE FAULT INTERRUPTER CALLED.");
	
}

void invlpg(u32int addr)
{
	asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}
