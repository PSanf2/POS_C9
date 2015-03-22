#include <paging.h>

// external variables defined in the linker that are needed to know where the kernel resides
extern u32int start;
extern u32int end;

u32int kernel_start = (u32int) &start;
u32int kernel_end = (u32int) &end;

page_directory_type kernel_page_directory;
page_directory_type *current_page_directory;

static u8int *frames;
static u32int max_index;

void paging_initialize(struct multiboot *mboot_ptr)
{
	
	// don't bother to juggle the page directory around to save the 4KB
	// of space the boot script used setting up the 4MB pages. It's just
	// not really worth it. This will also allow me to dynamically
	// allocate space for the page directory w/ another function to make
	// this code position independent.
	
	// create the kernel page directory
	// create the kernel page tables (not just one, all that are needed)
	
	// set up the system to use 4KB pages
	
	// i'm going to set up the page directory at the first page aligned
	// address after the kernel.
	
	// gather information
	u32int mem_in_mb = mboot_ptr->mem_upper / 1024 + 2;
	u32int mem_in_kb = mem_in_mb * 1024;
	u32int num_frames = mem_in_kb / 4;
	
	u32int kernel_page_dir_virt_addr = (kernel_end & ~(0xFFF)) + 0x1000;
	u32int kernel_page_dir_phys_addr = kernel_page_dir_virt_addr - 0xC0000000;
	
	u32int kernel_page_table_virt_addr = kernel_page_dir_virt_addr + 0x1000;
	u32int kernel_page_table_phys_addr = kernel_page_table_virt_addr - 0xC0000000;
	
	
	// print out what i have for debugging
	put_str("\nkernel_start: ");
	put_hex(kernel_start);
	put_str("\nkernel_end: ");
	put_hex(kernel_end);
	put_str("\nmem_in_mb: ");
	put_dec(mem_in_mb);
	put_str("\nmem_in_kb: ");
	put_dec(mem_in_kb);
	put_str("\nnum_frames: ");
	put_dec(num_frames);
	put_str("\nkernel_page_dir_virt_addr: ");
	put_hex(kernel_page_dir_virt_addr);
	put_str("\nkernel_page_dir_phys_addr: ");
	put_hex(kernel_page_dir_phys_addr);
	put_str("\nkernel_page_table_virt_addr: ");
	put_hex(kernel_page_table_virt_addr);
	put_str("\nkernel_page_table_phys_addr: ");
	put_hex(kernel_page_table_phys_addr);
	
	
	// make a pointer to the place i'll be creating the page directory
	u32int *page_dir_ptr = (u32int *) kernel_page_dir_virt_addr;
	
	// empty out that 4KB of memory
	memset((u8int *) page_dir_ptr, 0, 4096);
	
	// create an empty page directory in there
	for (u32int i = 0; i < 1024; i++)
	{
		page_dir_ptr[i] = 0 | 2;
	}
	
	// make a pointer to the place i'll be creating my page table
	u32int *page_table_ptr = (u32int *) kernel_page_table_virt_addr;
	
	// empty out that 4KB of space
	memset((u8int *) page_table_ptr, 0, 4096);
	
	// create the mappings i'll need for that page table
	u32int phys_addr_ctr = 0x0;
	for (u32int i = 0; i < 1024; i++)
	{
		if (phys_addr_ctr <= kernel_page_table_phys_addr)
		{
			page_table_ptr[i] = phys_addr_ctr | 3;
		}
		else
		{
			page_table_ptr[i] = 0 | 2;
		}
		phys_addr_ctr += 0x1000;
	}
	
	// figure out the page directory index i'll need to work with.
	u32int kernel_page_dir_index = 0xC0000000 >> 22;
	
	// put the physical address of the page table on the page directory at the proper index.
	page_dir_ptr[kernel_page_dir_index] = kernel_page_table_phys_addr | 3;
	
	// set up the recursive mappings
	// FFC0 0000 = page table 0 virt addr
	// FFC0 1000 = page table 1
	// FFC0 2000 = page table 2
	// etc...
	// FFFF F000 = page table 1023
	page_dir_ptr[1023] = kernel_page_dir_phys_addr | 3;
	
	// save the virtual address and physical address of the page directory
	kernel_page_directory.virt_addr = (u32int *) kernel_page_dir_virt_addr;
	kernel_page_directory.phys_addr = kernel_page_dir_phys_addr;
	
	/*
	// print it
	put_str("\n&kernel_page_directory: ");
	put_hex((u32int) &kernel_page_directory);
	put_str("\nkernel_page_directory.virt_addr: ");
	put_hex((u32int) kernel_page_directory.virt_addr);
	put_str("\nkernel_page_directory.phys_addr: ");
	put_hex(kernel_page_directory.phys_addr);
	*/
	
	// set up the pointer for the current page directory
	current_page_directory = (page_directory_type *) &kernel_page_directory;
	
	/*
	// print it
	put_str("\ncurrent_page_directory: ");
	put_hex((u32int) current_page_directory);
	put_str("\ncurrent_page_directory->virt_addr: ");
	put_hex((u32int) current_page_directory->virt_addr);
	put_str("\ncurrent_page_directory->phys_addr: ");
	put_hex(current_page_directory->phys_addr);
	*/
	
	// figure out the new cr4 value i'll need
	u32int new_cr4_val = read_cr4() & ~(0x00000010);
	
	// switch the system over to 4KB paging, and give it the phys addr of the new page dir
	asm volatile (
		"mov %0, %%cr3\n\t"
		"mov %1, %%cr4"
		: /* no outputs */
		: "r" (kernel_page_dir_phys_addr), "r" (new_cr4_val)
	);
	
	// enable paging (just to make sure i've done it)
	write_cr0((u32int) (read_cr0() | 0x80000000));
	
	// set up the bitmap.
	/*
	 * The bitmap will take up a theoretical maximum of 1MB of space
	 * The bitmap will be located in the physical memory right after the kernel page table.
	 * The bitmap will be mapped to the virtual address range of 0xFFB00000 to 0xFFBFFFFF.
	 * The bitmap's page directory will be in PD[1022].
	 */
	
	// figure out the virtual and physical address for the bitmap page table
	u32int bitmap_page_table_virt_addr = kernel_page_table_virt_addr + 0x1000;
	u32int bitmap_page_table_phys_addr = bitmap_page_table_virt_addr - 0xC0000000;
	
	/*
	// print it
	put_str("\nbitmap_page_table_virt_addr: ");
	put_hex(bitmap_page_table_virt_addr);
	put_str("\nbitmap_page_table_phys_addr: ");
	put_hex(bitmap_page_table_phys_addr);
	*/
	
	// figure out the virtual and physical address i'll use for the bitmap
	u32int bitmap_virt_addr = 0xFFB00000;
	u32int bitmap_phys_addr = bitmap_page_table_phys_addr + 0x1000;
	
	/*
	// print it
	put_str("\nbitmap_virt_addr: ");
	put_hex(bitmap_virt_addr);
	put_str("\nbitmap_phys_addr: ");
	put_hex(bitmap_phys_addr);
	*/
	
	// figure out the number of frames i'll need for the bitmap
	u32int bytes_needed_for_bitmap = num_frames / 8;
	u32int frames_needed_for_bitmap = bytes_needed_for_bitmap / 0x1000;
	
	if (bytes_needed_for_bitmap % 0x1000 != 0)
	{
		frames_needed_for_bitmap++;
	}
	
	/*
	//print it
	put_str("\nbytes_needed_for_bitmap: ");
	put_dec(bytes_needed_for_bitmap);
	put_str("\nframes_needed_for_bitmap: ");
	put_dec(frames_needed_for_bitmap);
	*/
	
	// create a page table for the bitmap.
	// figure out the index on the page table to use
	u32int table_index = (bitmap_page_table_virt_addr >> 12) & 0x3FF;
	
	/*
	// print it
	put_str("\ntable_index: ");
	put_dec(table_index);
	*/
	
	// set it as present on the page table
	page_table_ptr[table_index] = bitmap_page_table_phys_addr | 3;
	
	// invalidate the buffer for it
	invlpg(bitmap_page_table_virt_addr);
	
	// make a pointer to the page table
	page_table_ptr = (u32int *) bitmap_page_table_virt_addr;
	
	// clear out the memory in there
	memset((u8int *) page_table_ptr, 0, 4096);
	
	// make a page table in there
	for (u32int i = 0; i < 1024; i++)
	{
		page_table_ptr[i] = 0 | 2;
	}
	
	// put the required pages on the page table
	// this is at a funny offset because the portion of the page directory
	// we're working w/ is in the last 3/4 of the 4MB space it can map.
	u32int bitmap_phys_addr_ctr = bitmap_phys_addr;
	for (u32int i = 768; i < (768 + frames_needed_for_bitmap); i++)
	{
		page_table_ptr[i] = bitmap_phys_addr_ctr | 3;
		bitmap_phys_addr_ctr += 0x1000;
	}
	
	// put the physical address of the bitmap page table on the page directory at the appropriate index.
	page_dir_ptr[1022] = bitmap_page_table_phys_addr | 3;
	
	// make a pointer to the bitmap
	frames = (u8int *) bitmap_virt_addr;
	
	// figure out the max index on that array.
	max_index = (num_frames / 64) - 1;
	
	/*
	// print it
	put_str("\nmax_index: ");
	put_dec(max_index);
	*/
	
	for (u32int i = 0; i < num_frames; i++)
	{
		clear_frame(i * 0x1000);
	}
	
	// go over the page directory, and page tables, to figure out which frames are being used
	
	// for each entry in the page directory
	for (u32int i = 0; i < 1024; i++)
	{
		// if there's a page table present
		if (page_dir_ptr[i] & 0x1)
		{
			// set the frame for the page table
			set_frame(page_dir_ptr[i] & ~(0xFFF));
			
			// figure out the virtual address for the page table's recursive mapping
			u32int virt_addr_base = 0xFFC00000;
			u32int virt_addr_offset = i << 12;
			u32int page_table_virt_addr = virt_addr_base + virt_addr_offset;
			
			// reset the page_table_ptr to that address
			page_table_ptr = (u32int *) page_table_virt_addr;
			
			// for each entry on the page table
			for (u32int j = 0; j < 1024; j++)
			{
				if (page_table_ptr[j] & 0x1)
				{
					set_frame(page_table_ptr[j] & ~(0xFFF));
				}
			}
			
		}
	}
	
	// register my interrupt handler
	register_interrupt_handler(14, (isr) &page_fault_interrupt_handler);
	
	put_str("\n");
}

void page_fault_interrupt_handler(__attribute__((unused)) registers regs)
{
	put_str("\nPage fault interrupted called.");
	put_str("\nFaulting virtual address: ");
	put_hex(read_cr2());
	put_str("\nHalting.");
	for (;;) {}
}

void invlpg(u32int addr)
{
	asm volatile ("invlpg (%0)" : : "b" (addr) : "memory");
}

void set_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	frames[index] |= (0x1 << offset);
}

void clear_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	frames[index] &= (0x1 << offset);
}

u32int test_frame(u32int frame_addr)
{
	u32int frame = frame_addr / 0x1000;
	u32int index = frame / 64;
	u32int offset = frame % 8;
	return (frames[index] & (0x1 << offset));
}

u32int first_free()
{
	for (u32int i = 0; i <= max_index; i++)
	{
		if (frames[i] != 0xFF)
		{
			for (u32int j = 0; j < 8; j++)
			{
				u32int test = (0x1 << j);
				if (!(frames[i] & test))
				{
					return ((i * 64) + j) * 0x1000;
				}
			}
		}
	}
	return 0xFFFFFFFF;
}
