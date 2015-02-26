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
	
	u32int num_page_tables = tot_pages / 1024;
	
	put_str("\nI'll end up with ");
	put_dec(num_page_tables);
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
	
	put_str("\nAddress of page_directory[0] is ");
	put_hex((u32int) &page_directory[0]);
	
	put_str("\nAddress of page_directory[1023] is ");
	put_hex((u32int) &page_directory[1023]);
	
	// i need to create a blank page directory
	// this gives me 1024 entries on the page table that are 0x00000002
	for (int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2;
	}
	
	// i need to create my page tables.
	/*
		
		CR3 = page directory address, 109000 (or the calculated address for the page directory + 0x1000
		
		structure of page directory
		 
		page_directory[0] = address to first page table (10A000)
		page_directory[1] = 10B000
		page_directory[2] = 10C000
		...
		page_directory[31] = address for 32nd page table
		page_directory[32] = 0 | 2
		...
		page_directory[1023] = 0 | 2
		
		structure for the page tables
		
		I'll have 32 page tables, each will be 4kb in size.
		memory will look like (assuming 128MB of memory):
		
		0x0 - 0xFFFFF = low memory (1MB used)
		
		0x100000 - 0x108D54 = kernel
		0x108D55 - 0x108FFF = space that's wasted to bump up to the next page aligned address (36KB used)
		
		0x109000 - 0x109FFF = page directory (4KB used)
		
		0x10A000 - 0x10AFFF = first page table
		0x10B000 - 0x10BFFF = second page table
		...
		0x? - 0x?FFF = 1024th page table (4MB used)
		
		Page tables should start around the 40KB + 5MB mark, around 0x50A000
		table_one[0] = 0x0
		table_one[1] = 0x1000
		table_one[2] = 0x2000
		...
		table_one[1023] = 0x900000? somewhere around there.
		
	 */
	
	// i need to create tot_pages / 1024 page tables, and put them in the page directory.
	// 1024 is the number of entries i can have on a page table
	// with 128MB of memory i should have 32 page tables
	// this means i'll have 32 entries on my page directory when i'm done.
	
	
	
	
	
	
	
	// attempting to create the first page table
	// this page table will cover the first 4KB of memory
	// this will be from address 0x0 to 0x1000
	
	u32int *page_table = (u32int *) ((u32int) &page_directory[1023] + 0x4);
	
	put_str("\nThe address of the first page table is ");
	put_hex((u32int) page_table);
	
	u32int addr_counter = 0x0;
	u32int dir_counter = 0;
	// while (or for?) loop here to create multiple page tables
	for (int i = 0; i < 1024; i++)
	{
		page_table[i] = addr_counter;
		addr_counter = addr_counter + 0x1000;
	}
	
	//put_str("\nValue of page_table[0] is ");
	//put_hex(page_table[0]);
	
	//put_str("\nValue of page_table[1] is ");
	//put_hex(page_table[1]);
	
	//put_str("\nValue of page_table[2] is ");
	//put_hex(page_table[2]);
	
	//put_str("\nValue of page_table[1021] is ");
	//put_hex(page_table[1021]);
	
	//put_str("\nValue of page_table[1022] is ");
	//put_hex(page_table[1022]);
	
	put_str("\nAddress of page_table[1023] is ");
	put_hex((u32int) &page_table[1023]);
	
	put_str("\nValue of page_table[1023] is ");
	put_hex(page_table[1023]);
	
	
	// i should now put the page table address in page_directory[0]
	page_directory[dir_counter] = (u32int) page_table;
	
	put_str("\nValue of page_directory[0] is ");
	put_hex(page_directory[0]);
	
	// end loop here
	
	
	
	
	
	
	
	
	put_str("\nDone initializing paging. Hope it worked!\n"); // remove when done
}

void page_fault_interrupt_handler(__attribute__ ((unused)) registers regs)
{
	
}
