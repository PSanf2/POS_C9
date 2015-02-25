#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

static u32int kernel_start = (u32int) &start;
static u32int kernel_end = (u32int) &end;

u32int *page_directory;

void paging_initialize(struct multiboot *mboot_ptr)
{
		// i'll want to pull in the mboot_ptr variable from the kernel main as a parameter
		/*
		1. Get a physical memory address where my page directory will reside (this is the first page aligned address of physical memory after the ? from above)
		2. Set all of the memory for that 4KB section to 0 to make sure there's no garbage in there.
		3. Get a physical memory address where my first page table will reside. (the second page aligned address of physical memory after the ?)
		4. Set all of the memory for that 4KB section to 0.
		5. Start treating the 4KB sections of memory as arrays.
		6. For each item on the page directory array set the attributes to 0 | 2.
		7. For the 0th item on the page directory set the attributes to the address of the page table | 3.
		8. Identity map all of the physical memory from 0 on up to whatever's available into the page table (which will give me 4GB of virtual memory addresses, but only 128MB of them will actually be "real"). (How will I know which pages are present?)
		9. Set the interrupt handler for my page fault handler.
		10. Put the physical address of the page directory on the cr3 register.
		11. Enable paging by doing | 0x80000000 on cr0.

		From there I'll need to write the page fault handler, and functions to allocate, free, and map new pages as needed.
		 */
	
	put_str("\nInitializing paging... Good luck!"); // remove when done
	
	// make sure i got a memory map from the kernel
	// if i don't have a memory map
	if (!(mboot_ptr->flags & 0x40))
	{
		// throw a fit, and refuse to play any more.
		put_str("\nGRUB failed to provide a memory map. Unable to initialize paging.");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	// information gathering
	
	// grab the values that define the memory map provided by GRUB, stick them into a struct w/ friendly names
	memory_map mmap;
	mmap.addr = mboot_ptr->mmap_addr;
	mmap.length = mboot_ptr->mmap_length;
	
	put_str("\nHigh memory starts at ");
	put_hex(mmap.addr);
	
	put_str("\nMemory map length is ");
	put_hex(mmap.length);
	
	// figure out how much memory i have
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	
	put_str("\nSystem has ");
	put_dec(mem_in_mb);
	put_str(" MB of memory.");
	
	u32int mem_in_kb = mem_in_mb * 1024;
	
	put_str("\nSystem has ");
	put_dec(mem_in_kb);
	put_str(" KB of memory.");
	
	u32int mem_in_bytes = mem_in_kb * 1024;
	
	put_str("\nSystem has ");
	put_dec(mem_in_bytes);
	put_str(" bytes of memory.");
	
	u32int last_mem_addr = mem_in_bytes - 1;
	
	put_str("\nLast memory address should be ");
	put_hex(last_mem_addr);
	
	u32int tot_pages = mem_in_kb / 4;
	put_str("\nSystem has ");
	put_dec(tot_pages);
	put_str(" pages of memory to use.");
	
	put_str("\nI'll end up with ");
	put_dec(tot_pages);
	put_str(" / 1024 = ");
	put_dec(tot_pages / 1024);
	put_str(" page tables in my page directory.");
	
	put_str("\nKernel starts at ");
	put_hex(kernel_start);
	
	put_str("\nKernel ends at ");
	put_hex(kernel_end);
	
	// i should now have all the information i'll need to get started.
	
	// i need to figure out where i'll put my page directory.
	// this should be the first page aligned address after the end of the kernel.
	// assume the address where the kernel ends is not page aligned.
	u32int page_dir_addr = kernel_end;
	page_dir_addr &= ~(0xFFF);
	page_dir_addr = page_dir_addr + 0x1000;
	
	put_str("\nI'll put the page directory at ");
	put_hex(page_dir_addr);
	
	// make a pointer to the page directory
	page_directory = (u32int *) page_dir_addr;
	
	// i need to clear out the 4KB of memory where the page directory address starts
	memset((u8int *) page_directory, 0, 4096);
	
	// i need to create tot_pages / 1024 page tables, and put them in the page directory.
	// 1024 is the number of entries i can have on a page table
	// with 128MB of memory i should have 32 page tables
	// this means i'll have 32 entries on my page directory when i'm done.
	
	put_str("\nDone initializing paging. Hope it worked!\n"); // remove when done
}

void page_fault_interrupt_handler(registers regs)
{
	
}
