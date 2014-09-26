#include <paging.h>

static void (*page_fault_handler)(u8int *buf, u16int size) = NULL;

// end is defined in the linker script
extern u32int end;
static u32int placement_address = (u32int) &end;

void paging_initialize()
{

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













