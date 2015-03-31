#include <system.h>

#define MAX_TERMINAL_BUFFER_SIZE 4096

u32int initial_esp;

static char terminal_buffer[MAX_TERMINAL_BUFFER_SIZE];
static u16int terminal_buffer_length = 0;
static u16int terminal_last_put = 0;
static char terminal_seperator = '>';

int kernel_main(struct multiboot *mboot_ptr, u32int initial_stack)
{
	// the first thing i should do is initalize the paging.
	// this is where i should initialize paging
	//paging_initialize(mboot_ptr); // this looks like where i'm getting stuck
	//volatile u16int *vga = (u16int *) 0xC00B8000; while (0==0) *vga += 1; // doesn't work if i attempt to initialize paging.
	
	initial_esp = initial_stack;
	
	gdt_initialize();
	
	idt_initialize();
	
	memset((u8int *) &interrupt_handler, 0, sizeof(isr) * 256);
	
	paging_initialize(mboot_ptr);
	
	// initialize the virtual memory manager.
	vmm_initialize();
	
	initialize_tasking();
	
	enable_interrupts();
	
	timer_initialize(100);
	
	keyboard_initialize();
	
	keyboard_set_handler(kernel_keyboard_handler);
	
	vga_set_handler(kernel_vga_handler);
	
	
	
	
	
	
	// everything below here is stuff i do to get set up for the terminal
	// when i get in to multitasking i'll need to set up a process for
	// the kernel, set up a process for a shell, and get both of them
	// running.
	
	memset((u8int *) terminal_buffer, 0, MAX_TERMINAL_BUFFER_SIZE); // clear the terminal buffer (initalize it to 0 when we start running)
	
	set_text_color(LIGHT_GREY, BLUE);
	
	//clear_screen();
	
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
				put_str("\n");
				put_str(&terminal_buffer[token_size + 1]);
				put_str("\n");
			}
			else if (strcmp((string) token, "ticks") == 0)
			{
				put_str("\n");
				put_dec(get_tick());
				put_str("\n");
			}
			else if (strcmp((string) token, "clear") == 0)
			{
				clear_screen();
				put_str("\r");
			}
			
			else if (strcmp((string) token, "readFault") == 0)
			{
				u32int *ptr = (u32int *) 0xA0000000;
				u32int do_fault = *ptr;
				put_str("\n");
				put_hex(do_fault); // this should print whatever garbage is in logical address 0xBADC0DE
				put_str("\n");
				put_str("Done with read fault test.\n");
			}
			else if (strcmp((string) token, "writeFault") == 0)
			{
				u32int *ptr = (u32int *) 0xA0000000;
				*ptr = 0xBADC0DE;
				put_str("\n");
				put_hex(*ptr); // this should print 0xDEADC0DE
				put_str("\nDone with write fault test.\n");
			}
			else if (strcmp((string) token, "mapTest") == 0)
			{
				u32int virt_addr1 = 0xA0001000;
				u32int virt_addr2 = 0xA000F000;
				u32int phys_addr = 0x500000;
				
				map_page(virt_addr1, phys_addr);
				u32int *ptr1 = (u32int *) virt_addr1;
				*ptr1 = 0x1234;
				put_str("\nTest for virt_addr1: *ptr1 = ");
				put_hex(*ptr1);
				
				map_page(virt_addr2, phys_addr);
				u32int *ptr2 = (u32int *) virt_addr2;
				put_str("\nTest for virt_addr2: *ptr2 = ");
				put_hex(*ptr2);
				
				put_str("\n");
			}
			else if (strcmp((string) token, "unmapTest") == 0)
			{
				u32int virt_addr = 0xA0002000;
				u32int phys_addr = 0x600000;
				
				map_page(virt_addr, phys_addr);
				u32int *ptr1 = (u32int *) virt_addr;
				*ptr1 = 0x1234;
				put_str("\n*ptr1 = ");
				put_hex(*ptr1);
				
				unmap_page(virt_addr);
				u32int *ptr2 = (u32int *) virt_addr;
				put_str("\n*ptr2 = ");
				put_hex(*ptr2);
				
				map_page(virt_addr, phys_addr);
				u32int *ptr3 = (u32int *) virt_addr;
				put_str("\n*ptr3 = ");
				put_hex(*ptr3);
				
				put_str("\n");
			}
			else if (strcmp((string) token, "printFree") == 0)
			{
				print_all_free();
				put_str("\n");
			}
			else if (strcmp((string) token, "printUsed") == 0)
			{
				print_all_used();
				put_str("\n");
			}
			else if (strcmp((string) token, "malloc") == 0)
			{
				put_str("\n");
				
				u32int size = str_to_u32int(&terminal_buffer[token_size + 1]);
				
				u32int *malloc_ptr = malloc(size);
				
				put_str("\nmalloc_ptr=");
				
				put_hex((u32int) malloc_ptr);
				
				put_str("\n");
			}
			else if (strcmp((string) token, "malloc_above") == 0)
			{
				put_str("\n");
				
				u32int size = str_to_u32int(&terminal_buffer[token_size + 1]);
				
				u32int *malloc_ptr = malloc_above(size, 0xE0000000, 0x400);
				
				put_str("\nmalloc_ptr=");
				
				put_hex((u32int) malloc_ptr);
				
				put_str("\n");
			}
			else if (strcmp((string) token, "free") == 0)
			{
				u32int addr_to_free = hex_str_to_u32int(&terminal_buffer[token_size + 1]);
				
				free((u32int *) addr_to_free);
				
				put_str("\n");
			}
			else if (strcmp((string) token, "print_tasks") == 0)
			{
				print_task_list();
				
				put_str("\n");
			}
			else if (strcmp((string) token, "print_current_task") == 0)
			{
				print_current_task();
				put_str("\n");
			}
			else if (strcmp((string) token, "hex_convert") == 0)
			{
				u32int decNumber = hex_str_to_u32int(&terminal_buffer[token_size + 1]);
				
				put_str("\n");
				put_hex(decNumber);
				put_str(" = ");
				put_dec(decNumber);
				put_str("\n");
			}
			else if (strcmp((string) token, "fork") == 0)
			{
				fork();
			}
			
			// else if () {}
			// else if () {}
			else
			{
				put_str("\nUnknown command.\n");
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
