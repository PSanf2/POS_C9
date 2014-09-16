#include <multiboot.h>
#include <system.h>

u32int initial_esp;

int kernel_main(__attribute__ ((unused)) struct multiboot *mboot_ptr, u32int initial_stack)
{
	initial_esp = initial_stack;
	
	return 0;
}
