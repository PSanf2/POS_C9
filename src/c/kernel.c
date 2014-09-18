#include <multiboot.h>
#include <system.h>

u32int initial_esp;

static char terminal_buffer[4096];
static u16int buffer_length = 0;

int kernel_main(__attribute__ ((unused)) struct multiboot *mboot_ptr, u32int initial_stack)
{
	initial_esp = initial_stack;
	
	gdt_initialize();
	idt_initialize();
	memset((u8int *) &interrupt_handler, 0, sizeof(isr) * 256);
	
	set_text_color(LIGHT_GREY, BLUE);
	clear_screen();
	vga_buffer_put_str("Welcome to Patrick's Operating System!\n");
	
	enable_interrupts();
	timer_initialize(100);
	keyboard_initialize();
	keyboard_set_handler(kernel_keyboard_handler);
	vga_set_handler(kernel_vga_handler);
	
	/* works. this shows that the interrupts are working, and the timer is keeping track of system up time.
	for (;;)
	{
		u32int count = get_tick();
		put_str("Tick ");
		put_dec(count);
		put_str(" !\n");
	}
	*/
	
	/* works. this shows that the timer is working and we can make the machine wait
	put_str("Current tick is ");
	put_dec(get_tick());
	put_str("\nWaiting 1000 ticks");
	u32int end_tick = get_tick() + 1000;
	while (get_tick() < end_tick) {}
	put_str("\nDone.");
	put_str("\nCurrent tick is ");
	put_dec(get_tick());
	*/
	
	vga_buffer_put_char('>');
	
	for (;;)
	{
		// this is the main loop of the kernel
		keyboard_flush();
		
		
		if (buffer_length > 0)
		{
			int i;
			for (i = 0; i < buffer_length; i++)
			{
				vga_buffer_put_char(terminal_buffer[i]);
			}
			
			if (terminal_buffer[i-1] == '\n')
			{
				vga_buffer_put_str("Enter was pressed.");
				
				// i should now be able to evaluate the input in the buffer
				// i define a token as the string of characters between spaces.
				// i should now be able to evaluate the first token and use it to decide an action
				
				
				
				
				vga_buffer_put_str("\n>");
			}
			
			
			buffer_length = 0;
		}
		
		vga_flush();
	}
	
	return 0;
}

void kernel_keyboard_handler(u8int *buf, u16int size)
{
	for (int i = 0; i < size; i++)
		//vga_buffer_put_char((char) buf[i]); // this now needs to go to an input buffer
		terminal_buffer[buffer_length++] = (char) buf[i];
}

void kernel_vga_handler(u8int *buf, u16int size)
{
	for (int i = 0; i < size; i++)
		put_char((char) buf[i]);
}
