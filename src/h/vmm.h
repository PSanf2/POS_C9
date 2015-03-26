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

// a function to get the ball rolling
void vmm_initialize();

// functions i need to allocate memory
u32int *malloc(u32int size);
vmm_node *search_free(u32int size);
u32int highest_addr(vmm_list *list);
vmm_node *split_free(vmm_node *node, u32int size);

// functions i need to free memory
void free(u32int *virt_addr);
vmm_node *search_free_neighbor(vmm_node *node);
vmm_node *search_used(u32int virt_addr);

// functions for adding and removing nodes from lists
void insert_after(vmm_list *list, vmm_node *node, vmm_node *new_node);
void insert_before(vmm_list *list, vmm_node *node, vmm_node *new_node);
void insert_first(vmm_list *list, vmm_node *new_node);
void insert_last(vmm_list *list, vmm_node *new_node);
void remove(vmm_list *list, vmm_node *node);

// functions for maintianing the free list
vmm_node *search_adjacent_free();
void compact_free(vmm_node *node);
void compact_all_free();

// stuff for debugging
void print_node(vmm_node *node);
void print_all(vmm_list *list);
void print_all_free();
void print_all_used();

#endif














