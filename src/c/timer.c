#include <timer.h>

static u32int tick = 0;

void timer_initialize(u32int freq)
{
	register_interrupt_handler(IRQ0, &timer_interrupt_handler);
	
	u32int divisor = 1193180 / freq;
	outb(0x43, 0x36);
	outb(0x40, (divisor & 0xFF));
	outb(0x40, ((divisor >> 8) & 0xFF));
}

void timer_interrupt_handler(__attribute__ ((unused)) registers regs)
{
	tick++;
}

u32int get_tick()
{
	return tick;
}
