#include <irq.h>

void irq_handler(registers regs)
{
	if (regs.int_no >= 40)
	{
		outb(0xA0, 0x20);
	}
	outb(0x20, 0x20);
	
	if (interrupt_handler[regs.int_no] != 0)
	{
		isr handler = interrupt_handler[regs.int_no];
		handler(regs);
	}
}
