#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

static u32int kernel_start = (u32int) &start;
static u32int kernel_end = (u32int) &end;

static u32int *page_directory;
static u32int *page_table;

void paging_initialize(struct multiboot *mboot_ptr)
{
	put_str("\nInitializing paging...");
	
	// gather information.
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	u32int mem_in_kb = mem_in_mb * 1024;
	u32int tot_4kb_pages = mem_in_kb / 4;
	u32int page_directory_phys_addr = read_cr3();
	u32int page_directory_virt_addr = page_directory_phys_addr + 0xC0000000;
	
	// print out the information to make sure i have everything.
	put_str("\nMemory in MB: ");
	put_dec(mem_in_mb);
	put_str("\nMemory in KB: ");
	put_dec(mem_in_kb);
	put_str("\nTotal 4KB pages: ");
	put_dec(tot_4kb_pages);
	put_str("\nPage directory physical address: ");
	put_hex(page_directory_phys_addr);
	put_str("\nPage directory virtual address: ");
	put_hex(page_directory_virt_addr);
	put_str("\nKernel start: ");
	put_hex(kernel_start);
	put_str("\nKernel end: ");
	put_hex(kernel_end);
	// done printing initial information.
	
	// i need to initialize my bitmap to make sure it's cleared out, and
	// ready to start keeping track of which frames are being used.
	bitmap_initialize(tot_4kb_pages);
	
	// figure out the address that i want to use for my new page table.
	u32int new_page_table_virt_addr = kernel_end;
	new_page_table_virt_addr &= ~(0xFFF);
	new_page_table_virt_addr += 0x1000;
	
	// print it out.
	put_str("\nNew page table virt addr: ");
	put_hex(new_page_table_virt_addr);
	
	// figure out the physical address for the new page table.
	u32int new_page_table_phys_addr = new_page_table_virt_addr - 0xC0000000;
	
	// print it out.
	put_str("\nNew page table phys addr: ");
	put_hex(new_page_table_phys_addr);
	
	// get an address that i want to use for my temp page directory
	u32int temp_page_dir_virt_addr = new_page_table_virt_addr + 0x1000;
	
	// print it out
	put_str("\nTemp page directory virtual address: ");
	put_hex(temp_page_dir_virt_addr);
	
	// figure out the physical address for that temp page directory.
	u32int temp_page_dir_phys_addr = temp_page_dir_virt_addr - 0xC0000000;
	
	// print it out
	put_str("\nTemp page directory physical address: ");
	put_hex(temp_page_dir_phys_addr);
	
	// make a pointer to the temp page directory.
	u32int *temp_page_directory = (u32int *) temp_page_dir_virt_addr;
	
	// clear out 4kb of space for the page directory.
	memset((u8int *) temp_page_directory, 0, 4096);
	
	// make a page directory in there
	for (int i = 0; i < 1024; i++)
	{
		temp_page_directory[i] = 0 | 2;
	}
	
	// create a pointer to the new page table
	page_table = (u32int *) new_page_table_virt_addr;
	
	// clear out the 4kb of space needed for the page table.
	memset((u8int *) page_table, 0, 4096);
	
	
	
	
	
	
	// this area is going to start causing problems once the kernel gets
	// above 4MB in size. i won't be mapping the crap beyond the initial
	// 4MB, and i'll start getting page faults.
	// i'll need to make sure i keep all of the system vital stuff
	// below 4MB until i can start mapping other stuff in.
	
	// create a page table for the new mappings.
	u32int phys_addr_ctr = 0x0;
	for (int i = 0; i < 1024; i++)
	{
		if (phys_addr_ctr <= temp_page_dir_phys_addr)
		{
			page_table[i] = phys_addr_ctr | 3; // attributes supervisor, read/write, present
			set_frame(phys_addr_ctr);
		}
		else
		{
			page_table[i] = 0 | 2;
		}
		phys_addr_ctr += 0x1000;
	}
	
	
	
	
	
	
	
	
	// the kernel is being mapped to 0xC0000000 so i need to figure out the proper index on the page directory for that address.
	u32int kernel_page_dir_index = 0xC0000000 >> 22;
	
	// print it out.
	put_str("\nKernel page directory index: ");
	put_dec(kernel_page_dir_index);
	
	// put the physical address of the page table on the page directory
	temp_page_directory[kernel_page_dir_index] = new_page_table_phys_addr | 3; // attributes supervisor, read/write, present
	
	// print it out
	put_str("\ntemp_page_directory[");
	put_dec(kernel_page_dir_index);
	put_str("] => ");
	put_hex(temp_page_directory[kernel_page_dir_index]);
	
	// get the cr4 value
	u32int new_cr4_val = read_cr4();
	
	// print it
	put_str("\nnew cr4 val => ");
	put_hex(new_cr4_val);
	
	// figure out what the new value should be
	new_cr4_val &= ~(0x00000010);
	
	// print it
	put_str("\nnew cr4 val => ");
	put_hex(new_cr4_val);
	
	// i'll need to use an asm volatile statement here to move the new cr4 value, and the new page directory physical address into the control registers.
	// i can't call any c functions to do this because doing so causes the whole system to triple fault as soon as either value is changed.
	asm volatile (
		"mov %0, %%cr3\n\t"
		"mov %1, %%cr4"
		: /* no outputs */
		: "r" (temp_page_dir_phys_addr), "r" (new_cr4_val)
	);
	
	// enable paging (just to make sure i've done it)
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
	// i now need to reclaim the space used for the original page directory, and
	// set my actual page directory up in the proper place.
	page_directory = (u32int *) page_directory_virt_addr;
	
	// make sure i'm writing to the correct place
	put_str("\npage_directory[");
	put_dec(kernel_page_dir_index);
	put_str("] => ");
	put_hex(page_directory[kernel_page_dir_index]);
	
	// clear out the space
	memset((u8int *) page_directory, 0, 4096);
	
	// create  a page table in there
	for (int i = 0; i < 1024; i++)
	{
		page_directory[i] = 0 | 2;
	}
	
	// put the physical address of the page table on the page directory
	page_directory[kernel_page_dir_index] = new_page_table_phys_addr | 3; // attributes supervisor, read/write, present
	
	// make sure i did it
	put_str("\npage_directory[");
	put_dec(kernel_page_dir_index);
	put_str("] => ");
	put_hex(page_directory[kernel_page_dir_index]);
	
	// tell the system to use the new page directory
	write_cr3(page_directory_phys_addr);
	
	// i no longer need the physical page for the temp page directory, and
	// should tell the bitmap to free the frame.
	clear_frame(temp_page_dir_phys_addr);
	
	// register my interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	put_str("\nAddress of page fault interrupt hanlder: ");
	put_hex((u32int) &page_fault_interrupt_handler);
	
	put_str("\nFirst free fame: ");
	put_hex(first_free());
	
	put_str("\nPaging initialized.");
	
	//put_str("\nHalting");
	//for(;;) {}
}

void page_fault_interrupt_handler(__attribute__((unused)) registers regs)
{
	put_str("\nPage fault interrupt handler called.");
	
	put_str("\nFaulting virtual address: ");
	put_hex(read_cr2());
	
	put_str("\nHalting.");
	for (;;) {}
}

void invlpg(u32int addr)
{
	asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}
