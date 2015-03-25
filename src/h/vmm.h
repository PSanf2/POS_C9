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

// external variables declared in the paging.c file
extern page_directory_type *current_page_directory;

// i need a function to create a new address space
void create_virt_addr_space(page_directory_type *page_directory);

// i need a function to allocate space from the current address space
u32int *malloc(u32int size);

// i need a function to free allocations from the current address space
void free(u32int *virt_addr);

#endif
