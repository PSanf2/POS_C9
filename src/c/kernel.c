#include <system.h>

#define MAX_TERMINAL_BUFFER_SIZE 4096

u32int initial_esp;


static char terminal_buffer[MAX_TERMINAL_BUFFER_SIZE];
static u16int terminal_buffer_length = 0;
static u16int terminal_last_put = 0;
static char terminal_seperator = '>';

int kernel_main(struct multiboot *mboot_ptr, u32int initial_stack)
{
	initial_esp = initial_stack;
	
	gdt_initialize();
	idt_initialize();
	memset((u8int *) &interrupt_handler, 0, sizeof(isr) * 256);
	
	enable_interrupts();
	timer_initialize(100);
	keyboard_initialize();
	keyboard_set_handler(kernel_keyboard_handler);
	vga_set_handler(kernel_vga_handler);
	memset((u8int *) terminal_buffer, 0, MAX_TERMINAL_BUFFER_SIZE); // clear the terminal buffer (initalize it to 0 when we start running)
	
	// this is where i'll get paging set up.
	
	// the first thing i need to do is copy the memory map provided by grub.
	// this is done because the information on the multiboot header comes in at an address below 1M, and i'm going to reclaim that space for a stack.
	u8int mem_map[mboot_ptr->mmap_length]; // an array
	memcpy((u8int *) &mem_map, (u8int *) mboot_ptr->mmap_addr, mboot_ptr->mmap_length);
	
	memory_map mem_map_descriptor;
	mem_map_descriptor.addr = (u32int) &mem_map;
	mem_map_descriptor.length = mboot_ptr->mmap_length;
	
	// i need to initialize paging
	paging_initialize();
	page_fault_set_handler(kernel_page_fault_handler);
	
	// i need to initialize my memory manager
	memory_manager_initialize(mboot_ptr, mem_map_descriptor);
	
	set_text_color(LIGHT_GREY, BLUE);
	clear_screen();
	vga_buffer_put_str("Welcome to Patrick's Operating System!\n");
	vga_buffer_put_char(terminal_seperator);
	
	for (;;)
	{
		// this is the main loop of the kernel
		keyboard_flush();
		
		terminal();
		
		vga_flush();
	}
	
	return 0;
}

/*
 * This works, but will eventually need to be updated, and expanded.
 * I shouldn't be using a giant if/else if/else to decide what to do.
 * I should have a list of available commands in memory, and see if the typed command is on that list
 * The list should point to the function that will handle that command.
 * these commands might not be in active memory. they could point to something stored on the hard drive that needs to be loaded into memory
 * before it's executed.
 */
void terminal()
{
	// if there's anything in the buffer
	if (terminal_buffer_length > 0)
	{
		// if the last character typed was a newline
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
			
			// this is where i evaluate the token, and get ready to send the control elsewhere.
			if (strcmp((string) token, "echo") == 0)
			{
				vga_buffer_put_str("\n");
				vga_buffer_put_str(&terminal_buffer[token_size + 1]);
				vga_buffer_put_str("\n");
			}
			else if (strcmp((string) token, "ticks") == 0)
			{
				vga_buffer_put_str("\n");
				vga_buffer_put_dec(get_tick());
				vga_buffer_put_str("\n");
			}
			// there's an issue w/ how i'm doing this.
			// if i don't include the new line statement then the terminal character ends up pushed over about
			// two tabs worth of space. if i leave the new line, then the terminal cursor is one line to low, and not at the top of the screen.
			else if (strcmp((string) token, "clear") == 0)
			{
				clear_screen();
				vga_buffer_put_str("\r");
			}
			// else if () {}
			// else if () {}
			else
			{
				vga_buffer_put_str("\n");
				vga_buffer_put_str("Unknown command.");
				vga_buffer_put_str("\n");
			}
			
			// clear the buffer.
			memset((u8int *) terminal_buffer, 0, MAX_TERMINAL_BUFFER_SIZE);
			terminal_buffer_length = 0;
			terminal_last_put = 0;
			
			// print a new terminal character.
			vga_buffer_put_char(terminal_seperator);
		}
		// else if the last character typed was a backspace
		else if (terminal_buffer[terminal_buffer_length - 1] == '\b')
		{
			if (terminal_buffer_length > 0)
			{
				vga_buffer_put_str("\b \b");
				terminal_buffer_length = terminal_buffer_length - 2;
			}
			if (terminal_last_put > 0)
			{
				terminal_last_put--;
			}
		}
		// else anything else was typed.
		// do i need to check if it was a printable character?
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
}

void kernel_keyboard_handler(u8int *buf, u16int size)
{
	for (int i = 0; i < size; i++)
		terminal_buffer[terminal_buffer_length++] = (char) buf[i];
}

void kernel_vga_handler(u8int *buf, u16int size)
{
	for (int i = 0; i < size; i++)
		put_char((char) buf[i]);
}

void kernel_page_fault_handler(u8int *buf, u16int size)
{
	kernel_vga_handler(buf, size);
	for (;;) {}
}
