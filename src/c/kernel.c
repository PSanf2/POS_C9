#include <multiboot.h>
#include <system.h>

u32int initial_esp;

int kernel_main(__attribute__ ((unused)) struct multiboot *mboot_ptr, u32int initial_stack)
{
	initial_esp = initial_stack;
	
	gdt_initialize();
	idt_initialize();
	memset((u8int *) &interrupt_handler, 0, sizeof(isr) * 256);
	
	clear_screen();
	put_str("Welcome to Patrick's Operating System!\n");
	
	enable_interrupts();
	timer_initialize(100);
	keyboard_initialize();
	keyboard_set_handler(kernel_keyboard_handler);
	
	/* works
	for (;;)
	{
		u32int count = get_tick();
		put_str("Tick ");
		put_dec(count);
		put_str(" !\n");
	}
	*/
	
	for (;;)
	{
		keyboard_flush();
	}
	
	return 0;
}

void kernel_keyboard_handler(u8int *buf, u16int size)
{
	for (int i = 0; i < size; i++)
		put_char((char) buf[i]);
}
