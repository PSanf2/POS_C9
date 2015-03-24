/*
 * Each process will need to have its own virtual address space.
 * Each process will have its own "heap." A heap is just a piece of memory that the process has had allocated to it.
 * Each process will have it's own unique page directory.
 * Each unique page directory will need to have the kernel mapped into it.
 * Each unique page directory will be recursively mapped.
 * Virtual address space above the 3G line will be "kernel space."
 * Everything below the 3G line will be "user space."
 * The kmalloc function will allocate memory to a given heap in pages.
 * The malloc and free functions will be used to allocate space from a process's heap.
 * The kernel will need to keep track of every process.
 */






#ifndef __VMM_H
#define __VMM_H

#include <system.h>

// forward declaration for type in paging.h
typedef struct page_directory_struct page_directory_type;

// forward declaration for type to be declared
typedef struct vmm_node_struct vmm_node_type;

// declarion of a list node
typedef struct vmm_node_struct
{
	vmm_node_type *prev;
	u32int *virt_addr;
	u32int size;
	vmm_node_type *next; 
};

typedef struct vmm_list_struct
{
	vmm_node_type *first;
	vmm_node_type *last;
} vmm_list_type;

 // i need a function to create a new address space
	// this function will accept a pointer to a page_directory_type
	// this function will need to create a new page directory for the address space
	// create the mappings in it to keep the kernel mapped
	// create the mappings to keep the bitmap mapped
	// allocate a frame of physical memory to store the list in
	// temporaily map that frame to some virtual address, and use it to create a list in there
	// set up the new mappings to keep the list at a predefined virtual address that malloc and free will use when working w/ a list.
	
	// I think i'll put the list at virt addr 0xFF800000. that'll be PD[1022], PT[0]. That'll give me 3MB of virtual address space where the list can live
	// since i'll only be storing the free allocations, and compacting them, the allocations for a process would need to get fragmented to shit before it caused
	// issues and hit the bitmap. I'd have to have more than 24,000 items on the list before it would start causing an issue.
	// if it did create an issue then it would clobber the bitmap, and fubar the physical memory manager.
void create_virt_addr_space(page_directory_type *page_directory);
	
 // i need a function to allocate space from the current address space
 u32int *malloc(u32int size);
 
 // i need a function to free allocations from the current address space
 void free(u32int *virt_addr);

#endif
