#include <isr.h>

void register_interrupt_handler(u8int n, isr handler)
{
		interrupt_handler[n] = handler;
}

void isr_handler(registers regs)
{
	if (interrupt_handler[regs.int_no] != 0)
	{
		isr handler = interrupt_handler[regs.int_no];
		handler(regs);
	}
}
