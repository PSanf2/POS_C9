#include <paging.h>

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

page_directory *kernel_directory = 0;
page_directory *current_directory = 0;

u32int *frames;
u32int nframes;

static void (*page_fault_handler)(u8int *buf, u16int size) = NULL;

// end is defined in the linker script
extern u32int end;
static u32int placement_address = (u32int) &end;

u32int paging_malloc_int(u32int size, int align, u32int *phys)
{
	if (align == 1 && (placement_address & 0xFFFFF000))
	{
		placement_address &= 0xFFFFF000;
		placement_address += 0x1000;
	}
	if (phys)
	{
		*phys = placement_address;
	}
	u32int tmp = placement_address;
	placement_address += size;
	return tmp;
}

void paging_initialize()
{
	u32int mem_end_page = 0x1000000;
	nframes = mem_end_page / 0x1000;
	frames = (u32int *) paging_malloc_int(INDEX_FROM_BIT(nframes), 0, 0);
	memset((u8int *) frames, 0, INDEX_FROM_BIT(nframes));
	
	kernel_directory = (page_directory *) paging_malloc_int(sizeof(page_directory), 1, 0);
	current_directory = kernel_directory;
	
	u32int i = 0;
	while (i < placement_address)
	{
		alloc_frame(get_page(i, 1, kernel_directory), 0, 0);
		i += 0x1000;
	}
	
	register_interrupt_handler(14, page_fault_interrupt_handler);
	
	switch_page_directory(kernel_directory);
}

page *get_page(u32int address, int make, page_directory *dir)
{
	address /= 0x1000;
	u32int table_idx = address / 1024;
	if (dir->tables[table_idx])
	{
		return &dir->tables[table_idx]->pages[address % 1024];
	}
	else if (make)
	{
		u32int tmp;
		dir->tables[table_idx] = (page_table *) paging_malloc_int(sizeof(page_table), 1, &tmp);
		dir->tablesPhysical[table_idx] = tmp | 0x7;
		return &dir->tables[table_idx]->pages[address % 1024];
	}
	else
	{
		return 0;
	}
	
}

void switch_page_directory(page_directory *dir)
{
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    u32int cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

static u32int first_frame()
{
	u32int i;
	u32int j;
	u32int val = 0;
	u8int stop = 0;
	for (i = 0; (i < INDEX_FROM_BIT(nframes)) && !stop; i++)
	{
		if (frames[i] != 0xFFFFFFFF)
		{
			for (j = 0; (j < 32) && !stop; j++)
			{
				u32int toTest = 0x1 << j;
				if (!(frames[i] & toTest))
				{
					val = i * 4 * 8 + j;
					stop = 1;
				}
			}
		}
	}
	return val;
}

static void set_frame(u32int frame_addr)
{
	u32int frame = frame_addr/0x1000;
    u32int idx = INDEX_FROM_BIT(frame);
    u32int off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

void alloc_frame(page *my_page, int is_kernel, int is_writeable)
{
	if (my_page->frame != 0)
	{
		return;
	}
	else
	{
		u32int idx = first_frame();
		if (idx == (u32int) -1)
		{
			// PANIC! no free frames.
			// i need to put in the panic function.
		}
		set_frame(idx * 0x1000);
		my_page->present = 1;
		my_page->rw = (is_writeable)?1:0;
		my_page->user = (is_kernel)?0:1;
		my_page->frame = idx;
	}
}

void page_fault_set_handler(void (*callback)(u8int *buf, u16int size))
{
	page_fault_handler = callback;
}

void page_fault_interrupt_handler(__attribute__ ((unused)) registers regs)
{
	// i need to gather the information, put it in a string, and use the callback to get the kernel to handle it.
}
