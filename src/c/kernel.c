#include <multiboot.h>
#include <system.h>

#define MAX_TERMINAL_BUFFER_SIZE 4096

u32int initial_esp;

static char terminal_buffer[MAX_TERMINAL_BUFFER_SIZE];
static u16int terminal_buffer_length = 0;
static u16int terminal_last_put = 0;
static char terminal_seperator = '>';

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
	memset((u8int *) terminal_buffer, 0, MAX_TERMINAL_BUFFER_SIZE); // clear the terminal buffer (initalize it to 0 when we start running)
	
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
	
	vga_buffer_put_char(terminal_seperator);
	
	for (;;)
	{
		// this is the main loop of the kernel
		keyboard_flush();

		if (terminal_buffer_length > 0)
		{
			if (terminal_buffer[terminal_buffer_length - 1] == '\n')
			{
				// replace the \n w/ a null terminator to turn the buffered characters into a "string"
				terminal_buffer[terminal_buffer_length - 1] = '\0';
				// get the first token off the buffer
				int token_size;
				char token[MAX_TERMINAL_BUFFER_SIZE];
				// for each character on the buffer
				for (token_size = 0; token_size < terminal_buffer_length; token_size++)
				{
					// if it's a space, new line, or null terminator
					if (terminal_buffer[token_size] == ' ' || terminal_buffer[token_size] == '\0')
					{
						// put a null terminator on the token
						token[token_size] = '\0';
						// we're done
						break;
					}
					// put it on the token
					token[token_size] = terminal_buffer[token_size];
				}
				
				vga_buffer_put_str("\n");
				vga_buffer_put_str(token); // works
				
				// this is where i evaluate the token, and get ready to send the control elsewhere.
				
				// clear the buffer.
				memset((u8int *) terminal_buffer, 0, MAX_TERMINAL_BUFFER_SIZE);
				terminal_buffer_length = 0;
				terminal_last_put = 0;
				
				// move the cursor to the next line, and reprint the terminal character
				vga_buffer_put_str("\n");
				vga_buffer_put_char(terminal_seperator);
			}
			else
			{
				// should i put the last character on the vga buffer?
				if (terminal_last_put < terminal_buffer_length)
				{
					vga_buffer_put_char(terminal_buffer[terminal_buffer_length - 1]);
					terminal_last_put++;
				}
			}

		}
		
		vga_flush();
	}
	
	return 0;
}

void kernel_keyboard_handler(u8int *buf, u16int size)
{
	for (int i = 0; i < size; i++)
		//vga_buffer_put_char((char) buf[i]); // this now needs to go to an input buffer
		terminal_buffer[terminal_buffer_length++] = (char) buf[i];
}

void kernel_vga_handler(u8int *buf, u16int size)
{
	for (int i = 0; i < size; i++)
		put_char((char) buf[i]);
}
