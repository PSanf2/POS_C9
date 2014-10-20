#include <memory_manager.h>
 
// external variables define in the linker.
extern u32int start;
extern u32int end;

static u32int kernel_start = (u32int) &start;
static u32int kernel_end = (u32int) &end;

u32int *stack_low;
u32int *stack_ptr;
u32int *stack_high;

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
	stack_ptr -= 1;
	*stack_ptr = addr;
}

u32int pop_physical_address()
{
	if (stack_ptr == stack_high)
	{
		vga_buffer_put_str("\nPhysical memory manager stack is empty.");
		vga_buffer_put_str("\nHalting.");
		for (;;) {}
	}
	u32int addr = *stack_ptr;
	stack_ptr += 1;
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
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	vga_buffer_put_dec(mem_in_mb);
	vga_buffer_put_str("MB");
	
	vga_buffer_put_str("\nKernel starts at ");
	vga_buffer_put_hex(kernel_start);
	vga_buffer_put_str(" Kernel ends at ");
	vga_buffer_put_hex(kernel_end);
	
	vga_buffer_put_str("\nmmap_addr=");
	vga_buffer_put_hex(mmap.addr);
	vga_buffer_put_str(" mmap_length=");
	vga_buffer_put_hex(mmap.length);
	
	vga_buffer_put_str("\n");
	vga_buffer_put_dec(mem_in_mb);
	vga_buffer_put_str("MB of memory provides ");
	u32int mem_in_kb = mem_in_mb * 1024;
	vga_buffer_put_dec(mem_in_kb);
	vga_buffer_put_str("KB.");
	
	u32int stack_size = mem_in_kb / 4;
	vga_buffer_put_str("\nThe stack will need to hold ");
	vga_buffer_put_dec(stack_size);
	vga_buffer_put_str(" entries to store every 4096th address.");
	
	stack_low = (u32int *) (kernel_end + 1);
	vga_buffer_put_str("\nstack_low=");
	vga_buffer_put_hex((u32int) stack_low);
	
	stack_high = (u32int *) (stack_low + stack_size);
	vga_buffer_put_str(" stack_high=");
	vga_buffer_put_hex((u32int) stack_high);
	
	stack_ptr = stack_high;
	vga_buffer_put_str(" stack_ptr=");
	vga_buffer_put_hex((u32int) stack_ptr);
	
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
			vga_buffer_put_str("\nRegion mapped to stack. Stack pointer = ");
			vga_buffer_put_hex((u32int) stack_ptr);
		}
		
		i += *size + 4;
	}
	
	vga_buffer_put_str("\n");
}
