#include <memory_manager.h>

u32int *stack_ptr;
u32int *initial_stack_ptr;

void push_physical_page(u32int addr)
{
	if (stack_ptr == 0x0)
	{
		vga_buffer_put_str("\nStack of free pages is full.");
		vga_buffer_put_str("\nHalted while attempting to push physical page to stack of free pages.");
		for (;;) {}
	}
	stack_ptr -= 4;
	*stack_ptr = addr;
}

u32int pop_physical_page()
{
	if (stack_ptr == initial_stack_ptr)
	{
		vga_buffer_put_str("\nStack of free pages is empty.");
		vga_buffer_put_str("\nHalted while attempting to pop physical page from stack of free pages.");
		for (;;) {}
	}
	u32int addr = *stack_ptr;
	stack_ptr += 4;
	return addr;
}

u32int map_page(u32int virt_addr, u32int phys_addr)
{
	if (virt_addr & 0xFFF)
	{
		virt_addr &= 0xFFFFF000;
	}
	if (phys_addr & 0xFFF)
	{
		phys_addr &= 0xFFFFF000;
	}
	
	u32int *page_dir_addr = (u32int *) read_cr3();
	u32int *page_dir_entry_addr = page_dir_addr + (virt_addr >> 22);
	
	u32int *page_table_addr = (u32int *) (*page_dir_entry_addr >> 12);	// this might cause me a problem. The example code shifted to the right, then to the left.
	
	// is the page table not present
	if ((u32int) *page_table_addr == 0)
	{
		// i need to create a new page table, create an entry in the page directory for it, and some other stuff.
		vga_buffer_put_str("\nPage table not present.");
		vga_buffer_put_str("\nHalted while attempting to map a page.");
		vga_buffer_put_str(" virt_addr = ");
		vga_buffer_put_dec(virt_addr);
		vga_buffer_put_str(" phys_addr = ");
		vga_buffer_put_dec(phys_addr);
		for (;;) {}
	}
	
	u32int *page_table_entry_addr = page_table_addr + ((virt_addr >> 12) & 0x3FF);
	*page_table_entry_addr = (u32int) (phys_addr | ((u32int) page_table_entry_addr & 0xFFF));
	
	// remove the page table from the translation lookaside buffer
	asm volatile ("invlpg (%0)" : : "b" (virt_addr) : "memory");
	
	// return 0 to signify success.
	return 0;
}

u32int map_pages(u32int virt_addr, u32int phys_addr, u32int count)
{
	if (virt_addr & 0xFFF)
	{
		virt_addr &= 0xFFFFF000;
	}
	if (phys_addr & 0xFFF)
	{
		phys_addr &= 0xFFFFF000;
	}
	for (u32int i = 0; i < count; i++)
	{
		map_page((virt_addr + count), (phys_addr + count));	// this is based off the example code. should it be + count or + i?
	}
	return 0; // return 0 to signify success.
}

void switch_page_directory(u32int *addr)
{
	write_cr3((u32int) addr);
}

u32int *create_page_table(u32int count)
{
	u32int page_directory = read_cr3();
	
	if (count > 1023)
	{
		return 0;
	}
	
	u32int *phys_page = (u32int *) pop_physical_page();
	
	u32int first = count * 0x400000;
	
	for (int i = 0; i < 1024; i++)
	{
		u32int page = first + (i * 0x1000);
		phys_page[i] = page | 3;
	}
	
	u32int *page_directory_ptr = (u32int *) page_directory;
	page_directory_ptr[count] = (u32int) phys_page | 3;
	
	write_cr3(page_directory);
	
	return (u32int *) phys_page;
}

u32int alloc_pages(u32int *start_at, u32int count)
{
	// wtf is this being used for?
	u32int *page_dir = (u32int *) read_cr3();
	u32int page_dir_offset = (u32int) start_at >> 22;
	u32int __attribute__ ((unused)) page_table = page_dir[page_dir_offset];
	
	for (u32int i = (u32int) start_at; i < (u32int) start_at + count * 4096; i += 4096)
	{
		u32int *page_dir_addr = (u32int *) (i >> 22);
		u32int *page_table_entry_addr = (u32int *) (page_dir_addr + ((i >> 12) & 0x3FF));
		
		// if the page is already allocated
		if (*page_table_entry_addr & 0x200)
		{
			return 1; // return 1 to signify the requested page is already allocated.
		}
	}
	
	for (u32int i = (u32int) start_at; i < (u32int) start_at + count * 4096; i += 4096)
	{
		u32int *page_dir_addr = (u32int *) (i >> 22);
		u32int *page_table_entry_addr = (u32int *) (page_dir_addr + ((i >> 12) & 0x3FF));
		
		*page_table_entry_addr |= 0x200;
		if (i == (u32int) start_at)
		{
			*page_table_entry_addr &= ~(0x400);
		}
		else
		{
			*page_table_entry_addr |= 0x400;
		}
		
		asm volatile ("invlpg (%0)" : : "b" (i) : "memory");
	}
	
	return 0; // success
}

u32int free_pages(u32int *start_at)
{
	u32int *page_dir = (u32int *) read_cr3();
	u32int page_dir_offset = (u32int) start_at >> 22;
	u32int page_table = page_dir[page_dir_offset];
	
	// is the page table not present?
	if ((page_table & 1) == 0)
	{
		// i need to create a new page table, and create an entry in the page directory in for it
		vga_buffer_put_str("\nPage table not present.");
		vga_buffer_put_str("\nHalted while attempting to free a page.");
		vga_buffer_put_str(" start_at = ");
		vga_buffer_put_dec((u32int) start_at);
		for (;;) {}
	}
	
	u32int block_size = 0;
	
	for (u32int i = (u32int) start_at; ; i += 4096)
	{
		u32int *page_dir_addr = (u32int *) (i >> 22);
		u32int *page_table_entry_addr = (u32int *) (page_dir_addr + ((i >> 12) & 0x3FF));
		
		if (!(*page_table_entry_addr & 0x400))
		{
			if (block_size == 0)
			{
				block_size++;
			}
			else
			{
				break;
			}
		}
		else
		{
			block_size++;
		}
	}
	
	for (u32int i = (u32int) start_at; i < (u32int) start_at + block_size * 4096; i += 4096)
	{
		u32int *page_dir_addr = (u32int *) (i >> 22);
		u32int *page_table_entry_addr = (u32int *) (page_dir_addr + ((i >> 12) & 0x3FF)); 
		
		*page_table_entry_addr &= ~(0x200);
		
		asm volatile ("invlpg (%0)" : : "b" (i) : "memory");
	}
	
	return 0; // success
}

void memory_manager_initialize(struct multiboot *mboot_ptr, memory_map mem_map)
{
	vga_buffer_put_str("Initializing memory...");
	
	if (!(mboot_ptr->flags & 0x40))
	{
		vga_buffer_put_str("\nBootloader failed to provide a memory map.");
		vga_buffer_put_str("\nHalted while attempting to initialize memory manager.");
		for (;;) {}
	}
	
	vga_buffer_put_str("\nAmount of memory: ");
	vga_buffer_put_dec(mboot_ptr->mem_upper / 1024 + 2);
	vga_buffer_put_str("MB");
	
	vga_buffer_put_str("\nmmap_addr = ");
	vga_buffer_put_dec(mem_map.addr);
	vga_buffer_put_str(" mmap_length = ");
	vga_buffer_put_dec(mem_map.length);
	
	stack_ptr = (u32int *) 0x90000;
	initial_stack_ptr = (u32int *) 0x90000; // do i need to do this? it wasn't getting initialized in the example code
	
	u32int i = mem_map.addr;
	while (i < (mem_map.addr + mem_map.length))
	{
		u32int *size = (u32int *) i;
		u32int *base_addr_low = (u32int *) (i + 4);
		u32int *length_low = (u32int *) (i + 12);
		u32int *type = (u32int *) (i + 20);
		
		vga_buffer_put_str("\nsize = ");
		vga_buffer_put_dec((u32int) size);
		vga_buffer_put_str(" start = ");
		vga_buffer_put_dec((u32int) base_addr_low);
		vga_buffer_put_str(" end = ");
		vga_buffer_put_dec((u32int) base_addr_low + (u32int) length_low);
		
		vga_buffer_put_str(" type = ");
		if (*type == 1)
		{
			vga_buffer_put_str("free");
			
			for (u32int j = *base_addr_low; j < (*base_addr_low + *length_low); j+= 4096)
			{
				if (j < (u32int) stack_ptr)
				{
					continue;
				}
				if (j >= 0x100000 && j < 0x200000)
				{
					continue;
				}
				
				push_physical_page(j);
			}
		}
		else
		{
			vga_buffer_put_str("reserved");
		}
		
		i += *size + 4;
	}
	
	vga_buffer_put_str("\n");
}






