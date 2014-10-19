#include <memory_manager.h>

/*
 * This will be my physical memory manager that I need to have in place before I can implement paging.
 * 
 * I will be keeping a stack/list of pages.
 * Each page will be a 4K block of memory w/ a 4k aligned address
 * I'll keep the list of free pages on a stack. The stack will simply be a 32-bit variable, and i'll
 * put a number (representing a memory address) on it. To pop something off the stack I'll return the
 * current value, and decrement it.
 * 
 * The number of pages will depend on the amount of physical memory available.
 * I'll use the memory map to figure out where the memory holes are. holes cannot be allocated.
 * 
 * 
 */
 
// external variables define in the linker.
extern u32int start;
extern u32int end;

static u32int kernel_start = (u32int) &start;
static u32int kernel_end = (u32int) &end;

// this will be the stack where i store memory addresses.
u32int *stack_ptr = (u32int *) 0x90000; // will be the current stack pointer
u32int const *initial_stack_ptr = (u32int *) 0x90000; // will be the initial stack pointer

void push_physical_address(u32int addr)
{
	// if the stack is full
	if (stack_ptr == 0x0)
	{
		// whine about it, and refuse to play any more.
		vga_buffer_put_str("\nPhysical memory manager stack is full.");
		vga_buffer_put_str("\nHalting.");
		for (;;) {}
	}
	//vga_buffer_put_str("\nPush ");
	//vga_buffer_put_hex(addr);
	//vga_buffer_put_str(" stack_ptr=");
	//vga_buffer_put_hex((u32int) stack_ptr);
	stack_ptr -= 4;
	*stack_ptr = addr;
}

u32int pop_physical_address()
{
	if (stack_ptr == initial_stack_ptr)
	{
		vga_buffer_put_str("\nPhysical memory manager stack is empty.");
		vga_buffer_put_str("\nHalting.");
		for (;;) {}
	}
	u32int addr = *stack_ptr;
	stack_ptr += 4;
	return addr;
}

void free_block(u32int addr)
{
	push_physical_address(addr);
}

u32int allocate_block()
{
	return pop_physical_address();
}

void memory_manager_initialize(struct multiboot *mboot_ptr)
{
	// i need to get the memory map out of the multiboot struct
	// i need to get the description of the memory map. This should be in mboot_ptr->mmap_length, and mboot_ptr->mmap_addr.
	// if this information isn't there, i can't do crap, and need to halt the system
	// this should tell me where my memory begins, and how long it is.
	// the memory map consists of pointers to areas that can be used as structs containing information about each region of available memory (i think)
	vga_buffer_put_str("\nInitializing memory manager...");
	
	// if i don't have a memory map
	if (!(mboot_ptr->flags & 0x40))
	{
		// throw a fit, and refuse to play any more
		vga_buffer_put_str("\nGRUB failed to provide a memory map.");
		vga_buffer_put_str("\nHalting");
		for (;;) {}
	}
	
	memory_map mmap;
	mmap.addr = mboot_ptr->mmap_addr;
	mmap.length = mboot_ptr->mmap_length;
	
	vga_buffer_put_str("\nAmount of memory: ");
	vga_buffer_put_dec(mboot_ptr->mem_upper / 1024 + 2);
	vga_buffer_put_str("MB");
	
	vga_buffer_put_str("\nKernel starts at ");
	vga_buffer_put_hex(kernel_start);
	vga_buffer_put_str(" Kernel ends at ");
	vga_buffer_put_hex(kernel_end);
	
	vga_buffer_put_str("\nmmap_addr=");
	vga_buffer_put_hex(mmap.addr);
	vga_buffer_put_str(" mmap_length=");
	vga_buffer_put_hex(mmap.length);
	
	// i need to figure out where i want to put my stack, and how big i'll need it to be.
	
	u32int i = mmap.addr;
	while (i < (mmap.addr + mmap.length))
	{
		u32int *size = (u32int *) i;
		u32int *base_addr = (u32int *) (i + 4);
		u32int *length = (u32int *) (i + 12);
		u32int *type = (u32int *) (i + 20);
		
		vga_buffer_put_str("\nsize=");
		vga_buffer_put_hex((u32int) *size);
		
		vga_buffer_put_str(" base_addr=");
		vga_buffer_put_hex((u32int) *base_addr);
		
		vga_buffer_put_str(" length=");
		vga_buffer_put_hex((u32int) *length);
		
		vga_buffer_put_str(" type=");
		vga_buffer_put_dec((u32int) *type);
		
		if (*type == 1)
		{
			vga_buffer_put_str("\nRegion is free. Stack pointer = ");
			vga_buffer_put_hex((u32int) stack_ptr);
			for (u32int j = *base_addr; j < (*base_addr + *length); j += 4096)
			{
				if (j < (u32int) stack_ptr)
				{
					continue;
				}
				if (j >= kernel_start && j < kernel_end)
				{
					continue;
				}
				push_physical_address(j);
			}
			vga_buffer_put_str("\nRegion mapped to stack. Stack pointer = ");
			vga_buffer_put_hex((u32int) stack_ptr);
		}
		
		i += *size + 4;
	}
	
	vga_buffer_put_str("\n");
}
