#include <vmm.h>

 // i need a function to create a new address space
	// this function will accept a pointer to a page_directory_type
	// create the mappings in it to keep the kernel mapped
	// create the mappings to keep the bitmap mapped
	// allocate a frame of physical memory to store the list in
	// temporaily map that frame to some virtual address, and use it to create a list in there
	// set up the new mappings to keep the list at a predefined virtual address that malloc and free will use when working w/ a list.
	
	// I think i'll put the list at virt addr 0xFF800000. that'll be PD[1022], PT[0]. That'll give me 3MB of virtual address space where the list can live
	// since i'll only be storing the free allocations, and compacting them, the allocations for a process would need to get fragmented to shit before it caused
	// issues and hit the bitmap. I'd have to have more than 24,000 items on the list before it would start causing an issue.
	// if it did create an issue then it would clobber the bitmap, and fubar the physical memory manager.
void create_virt_addr_space(__attribute__((unused)) page_directory_type *page_directory)
{
	// the virtual address for the page directory really only applies when that page directory is the current page directory.
	// i can't count on this being the case, so i should map the address somewhere.
	
}

// i need a function to allocate space from the current address space
u32int *malloc(__attribute__((unused)) u32int size)
{
	return 0;
}

// i need a function to free allocations from the current address space
void free(__attribute__((unused)) u32int *virt_addr)
{
	
}



