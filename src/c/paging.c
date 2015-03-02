#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

static u32int kernel_start __attribute__((unused)) = (u32int) &start; // unused for the moment, but i think i'll need it later
static u32int kernel_end = (u32int) &end;

void paging_initialize(struct multiboot *mboot_ptr)
{
	put_str("\nInitializing paging.");
	
	// make sure I have a memory map from the kernel.
	// if i don't have a memory map
	if (!(mboot_ptr->flags & 0x40))
	{
		// throw a fit, and refuse to play any more.
		put_str("\nGRUB failed to provide a memory map. Unable to initialize paging.");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	// gather information.
	
	// figure out how much memory i have in MB
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	
	// figure out how much that is in KB.
	u32int mem_in_kb = mem_in_mb * 1024;
	
	// figure out how much memory i have in bytes
	u32int mem_in_bytes = mem_in_kb * 1024;
	
	// figure out what the last physical memory address should be.
	u32int last_phys_mem_addr __attribute__((unused)) = mem_in_bytes - 1; // unused for the moment, but i think i'll need it later.
	
	// figure out how many page tables i'll need to create
	u32int tot_pages = mem_in_kb / 4;
	
	// figure out how many page tables i'll need
	u32int tot_page_tables = tot_pages / 1024;
	
	// quick test to make sure i have the right numbers.
	//put_str("\nTotal number of page tables I'll be creating is ");
	//put_dec(tot_page_tables);
	// worked
	
	// figure out where i'll put my page directory.
	// i'll be putting it at the first page aligned address after the kernel.
	u32int page_dir_addr = kernel_end;
	page_dir_addr &= ~(0xFFF);
	page_dir_addr = page_dir_addr + 0x1000;
	
	// create a pointer to the place in memory where the page directory will actually be.
	u32int *page_directory;
	page_directory = (u32int *) page_dir_addr;
	
	// clear out the 4KB where the page directory will live (just to make sure there's no garbage 'cause ya never know)
	memset((u8int *) page_directory, 0, 4096);
	
	// do a quick test to make sure it actually worked properly.
	//put_str("\nKernel ends at ");
	//put_hex(kernel_end);
	//put_str("\npage_directory[0] is at ");
	//put_hex((u32int) &page_directory[0]);
	// worked.
	
	// create a blank page directory.
	// this will give me 1024 entries on the array that have the value of 0 | 2
	// this appears to be correct as per http://wiki.osdev.org/Setting_Up_Paging
	// (if i'm only making 32 page tables then why am I marking all of the others? future mappings?)
	for (int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2; // supervisor, read/write, not present
	}
	
	// figure out where i'm going to start putting my page tables.
	// page tables will be put right after the page directory, and sequential
	// since page tables are 4KB this will also serve as a counter.
	u32int page_table_addr = (u32int) &page_directory[1023] + 0x4;
	
	// quick test to make sure everything's alright.
	//put_str("\nAddress of page_directory[1023] is ");
	//put_hex((u32int) &page_directory[1023]);
	//put_str("\nPage tables will start at ");
	//put_hex(page_table_addr);
	// worked
	
	// i'll also need a counter to keep track of the physical memory addresses
	u32int phys_addr_counter = 0x0;
	
	// i need to loop tot_page_tables times to create all the page tables i'll need.
	// for each page table...
	for (u32int i = 0; i < tot_page_tables; i++)
	{
		// create a pointer to the place in memory where the page table will be
		u32int *page_table;
		page_table = (u32int *) page_table_addr;
		
		// do a quick test.
		//put_str("\nPage table should be put at ");
		//put_hex((u32int) page_table);
		// worked
		
		// clear out the 4KB of memory that'll hold the page table (to make sure there's no garbage in there).
		memset((u8int *) page_table, 0, 4096);
		
		// for each entry on the page table
		for (int j = 0; j < 1024; j++)
		{
			// put the proper physical memory address on the array
			page_table[j] = phys_addr_counter | 3; // supervisor, read/write, present
			
			// if the address is not inside the kernel then it shouldn't be supervisor only.
			
			// advance the counter
			phys_addr_counter += 0x1000;
		}
		
		// doing some tests.
		/*
		for (int k = 0; k < 17; k++)
		{
			put_str("\nValue of page_table[");
			put_dec(k);
			put_str("] is ");
			put_hex((u32int) page_table[k]);
		}
		put_str("\nHalting.");
		for (;;) {}
		// works, confirms that the proper values are being put on the page tables.
		*/
		
		// put the page table address in page_directory[i]
		page_directory[i] = (u32int) page_table | 3; // read/write, present
		
		// if the page directory doesn't map to the kernel then it shouldn't be supervisor only.
		
		// increment the page table address counter.
		page_table_addr += 0x1000;
		
	}
	
	// doing a couple of quick tests
	
	/*
	// see what address is on the page directory at a given index
	put_str("\nValue of page_directory[0] is ");
	put_hex(page_directory[0]);
	// make a pointer to that value
	u32int *table_ptr;
	// adjust the value of the pointer (to make a pointer to the page table)
	table_ptr = (u32int *) (page_directory[0] & ~(0xFFF));
	// see what's in there.
	for (int k = 0; k < 16; k++)
	{
		put_str("\nValue at table_ptr[");
		put_dec(k);
		put_str("] is ");
		put_hex((u32int) table_ptr[k]);
	}
	put_str("\nLast memory address should be ");
	put_hex(last_phys_mem_addr);
	*/
	// I *think* this is working. i'm getting the expected values coming back from the page tables.
	// If I try to look at the contents of page_directory[32] (which should have nothing in it) then things get funny because
	// doing 0 & ~(0xFFF) causes the address i'm trying to look to go funky.
	
	/*
	for (int k = 0; k < 16; k++)
	{
		put_str("\nAddress of page_directory[");
		put_dec(k);
		put_str("] is ");
		put_hex((u32int) &page_directory[k]);
		put_str(". Value is ");
		put_hex((u32int) page_directory[k]);
	}
	*/
	// worked.
	
	/*
	for (int k = 16; k < 33; k++)
	{
		put_str("\nAddress of page_directory[");
		put_dec(k);
		put_str("] is ");
		put_hex((u32int) &page_directory[k]);
		put_str(". Value is ");
		put_hex((u32int) page_directory[k]);
	}
	*/
	// worked
	
	/*
	for (int k = 33; k < 49; k++)
	{
		put_str("\nAddress of page_directory[");
		put_dec(k);
		put_str("] is ");
		put_hex((u32int) &page_directory[k]);
		put_str(". Value is ");
		put_hex((u32int) page_directory[k]);
	}
	*/
	// worked
	
	// i need to set the interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	// i need to put the address of the page directory on CR3
	write_cr3(page_dir_addr);
	
	// i need to set the paging enable bit on CR0
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
	put_str("\nDone initializing paging.\n");
}

void page_fault_interrupt_handler(__attribute__ ((unused)) registers regs)
{
	
}












