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
	if (!(mboot_ptr->flags & 0x40))
	{
		// throw a fit, and refuse to play any more.
		put_str("\nGRUB failed to provide a memory map. Unable to initialize paging.");
		put_str("\nHalting.");
	}
	
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
	
	// get an address that i want to use for my new page directory
	u32int new_page_dir_virt_addr = kernel_end;
	new_page_dir_virt_addr &= ~(0xFFF);
	new_page_dir_virt_addr += 0x1000;
	
	// print it out
	put_str("\nNew page directory virtual address: ");
	put_hex(new_page_dir_virt_addr);
	
	// figure out the physical address for that new page directory.
	u32int new_page_dir_phys_addr = new_page_dir_virt_addr - 0xC0000000;
	
	// print it out
	put_str("\nNew page directory physical address: ");
	put_hex(new_page_dir_phys_addr);
	
	// make a pointer to the new page directory.
	page_directory = (u32int *) new_page_dir_virt_addr;
	
	// clear out 4kb of space for the page directory.
	memset((u8int *) page_directory, 0, 4096);
	
	// figure out the address that i want to use for my new page table.
	u32int new_page_table_virt_addr = new_page_dir_virt_addr + 0x1000;
	
	// print it out.
	put_str("\nNew page table virt addr: ");
	put_hex(new_page_table_virt_addr);
	
	// figure out the physical address for the new page table.
	u32int new_page_table_phys_addr = new_page_table_virt_addr - 0xC0000000;
	
	// print it out.
	put_str("\nNew page table phys addr: ");
	put_hex(new_page_table_phys_addr);
	
	// create a pointer to the new page table
	page_table = (u32int *) new_page_table_virt_addr;
	
	// clear out the 4kb of space needed for the page table.
	memset((u8int *) page_table, 0, 4096);
	
	// the kernel is being mapped to 0xC0000000 so i need to figure out the proper index on the page directory for that address.
	u32int kernel_page_dir_index = 0xC0000000 >> 22;
	
	// print it out.
	put_str("\nKernel page directory index: ");
	put_dec(kernel_page_dir_index);
	
	// create a page table for the new mappings.
	u32int phys_addr_ctr = 0x0;
	for (int i = 0; i < 1024; i++)
	{
		page_table[i] = phys_addr_ctr | 3; // attributes supervisor, read/write, present
		phys_addr_ctr += 0x1000;
	}
	
	// put the physical address of the page table on the page directory
	page_directory[kernel_page_dir_index] = new_page_table_phys_addr | 3; // attributes supervisor, read/write, present
	
	// print it out
	put_str("\npage_directory[");
	put_dec(kernel_page_dir_index);
	put_str("] => ");
	put_hex(page_directory[kernel_page_dir_index]);
	
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
		: "r" (new_page_dir_phys_addr), "r" (new_cr4_val)
	);
	
	// enable paging (just to make sure i've done it)
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
	// register my interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	put_str("\nAddress of page fault interrupt hanlder: ");
	put_hex((u32int) &page_fault_interrupt_handler);
	
	put_str("\nPaging initialized.\n");
	
	//put_str("\n");
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
