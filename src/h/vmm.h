#ifndef __VMM_H
#define __VMM_H

#include <system.h>

// forward declaration for type in paging.h
typedef struct page_directory_struct page_directory_type;

typedef struct vmm_node_struct
{
	struct vmm_node_struct *prev;
	u32int virt_addr;
	u32int size;
	struct vmm_node_struct *next;
} vmm_node;

typedef struct vmm_list_struct
{
	struct vmm_node_struct *first;
	struct vmm_node_struct *last;
} vmm_list;

// external variables declared in the paging.c file
extern page_directory_type *current_page_directory;

void vmm_initialize();

void print_node(vmm_node *node);

// i need a function to allocate space from the current address space
u32int *malloc(u32int size);

// i need a function to free allocations from the current address space
void free(u32int *virt_addr);

// functions needed to handle the list
void insert_after(vmm_node *node, vmm_node *new_node);
void insert_before(vmm_node *node, vmm_node *new_node);
void insert_beginning(vmm_node *node);
void insert_end(vmm_node *node);
void remove(vmm_node *node);
vmm_node *split(vmm_node *node, u32int bytes);
vmm_node *search_adjacent_free();
void compact_after(vmm_node *node);
void compact_all();

#endif














