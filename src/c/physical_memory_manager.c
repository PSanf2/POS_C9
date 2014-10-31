#include <physical_memory_manager.h>
 
// external variables defined in the linker.
extern u32int start;
extern u32int end;

static u32int kernel_start = (u32int) &start;
static u32int kernel_end = (u32int) &end;

// variables used to define a stack.
u32int *stack_low;
u32int *stack_ptr = 0;
u32int *stack_high; // stack high getting set to 0 is being used as a flag value.

void push_physical_address(u32int addr)
{
	// if the stack is full
	if (stack_ptr == stack_low)
	{
		// whine about it, and refuse to play any more.
		vga_buffer_put_str("\nPhysical memory manager stack is full.");
		vga_buffer_put_str("\nHalting.");
		for (;;) {}
	}
	
	stack_ptr--;
	*stack_ptr = addr;
}

u32int pop_physical_address()
{
	// if the stack is empty
	if (stack_ptr == stack_high)
	{
		// whine about it and refuse to play any more.
		vga_buffer_put_str("\nPhysical memory manager stack is empty.");
		vga_buffer_put_str("\nHalting.");
		for (;;) {}
	}
	
	
	u32int addr = *stack_ptr;
	stack_ptr++;
	return addr;
}

u32int mm_stack_full()
{
	return (stack_ptr == stack_low);
}

u32int mm_stack_empty()
{
	return (stack_ptr == stack_high);
}

// wrapper functions to be used by outside calls.
void free_block(u32int addr)
{
	push_physical_address(addr);
}

u32int allocate_block()
{
	if (!mm_stack_empty())
	{
		return pop_physical_address();
	}
	else
	{
		return 0;
	}
}

// get the ball rollin'
void memory_manager_initialize(struct multiboot *mboot_ptr)
{
	// if i don't have a memory map
	if (!(mboot_ptr->flags & 0x40))
	{
		// throw a fit, and refuse to play any more
		vga_buffer_put_str("\nGRUB failed to provide a memory map.");
		vga_buffer_put_str("\nHalting");
		for (;;) {}
	}
	
	// grab the two values that define the memory map provided by GRUB,
	// and stick them in a struct with friendlier names.
	memory_map mmap;
	mmap.addr = mboot_ptr->mmap_addr;
	mmap.length = mboot_ptr->mmap_length;
	
	// figure out how much memory i have.
	// ya gotta twiddle the math for some reason. idunnolawl
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;

	// figure how how much total memory i have in kilobytes.
	u32int mem_in_kb = mem_in_mb * 1024;
	
	// how many 4KB blocks of memory will i have?
	u32int stack_size = mem_in_kb / 4;
	
	u32int i = mmap.addr;
	while (i < (mmap.addr + mmap.length))
	{
		u32int *size = (u32int *) i;
		u32int *base_addr = (u32int *) (i + 4);
		u32int *length = (u32int *) (i + 12);
		u32int *type = (u32int *) (i + 20);
		
		if ((*type == 1) && (*base_addr >= 0x100000))
		{
			// i should get the finishing address of the region, maybe?
			
			u32int my_base_addr = *base_addr; // don't overwrite the *base addr or you'll have a bad time.
			u32int end_addr = (*base_addr + *length) - 1;
			
			// is the address inside the kernel?
			if (my_base_addr >= kernel_start && my_base_addr < kernel_end)
			{
				while ((my_base_addr >= kernel_start && my_base_addr < kernel_end))
				{
					my_base_addr = my_base_addr + 1;
				}
			}
			
			// make sure the address is page aligned.
			if (my_base_addr & 0xFFF)
			{
				my_base_addr &= ~(0xFFF);
				my_base_addr = my_base_addr + 0x1000;
			}
			
			if ((end_addr - my_base_addr) > (stack_size * sizeof(u32int)))
			{
				// there's enough space
				stack_low = (u32int *) my_base_addr;
				stack_high = (u32int *) (stack_low + stack_size);
				stack_ptr = stack_high;
				// i'm done
				break;
			}
			
		}
		
		i += *size + 4;
	}
		
	i = mmap.addr;
	while (i < (mmap.addr + mmap.length))
	{
		u32int *size = (u32int *) i;
		u32int *base_addr = (u32int *) (i + 4);
		u32int *length = (u32int *) (i + 12);
		u32int *type = (u32int *) (i + 20);
		
		if (*type == 1)
		{
			if (*base_addr >= 0x100000)
			{
				u32int j = *base_addr;
				// make sure the address is 4K aligned
				if (j & 0xFFF)
				{
					j &= ~(0xFFF); // set the last twelve bits of the pointer to 0 (this causes the pointer to back up!)
					j = j + 0x1000; // advance the pointer, don't back it up.
				}
				
				for (; j < (*base_addr + *length); j += 4096)
				{
					if (j >= (u32int) stack_low && j < (u32int) stack_high)
					{
						continue;
					}
					if (j >= kernel_start && j < kernel_end)
					{
						continue;
					}
					push_physical_address(j);
				}
			}
		}
		
		i += *size + 4;
	}

}
