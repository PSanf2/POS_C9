#ifndef __VMM_H
#define __VMM_H

#include <system.h>

// forward declarations
typedef struct page_directory_struct page_directory_type;
typedef struct list_struct list_type;
typedef struct list_node_struct list_node_type;

// external variables declared in the paging.c file
extern volatile page_directory_type *current_page_directory;

typedef struct allocation_struct
{
	u32int virt_addr;
	u32int size;
} allocation_type;

// a function to get the ball rolling
void vmm_initialize();
void print_allocation_node(list_node_type *node);
void print_allocation_list(list_type *list);
void compact_after(list_node_type *node);
list_node_type *search_adjacent_free();
void compact_all_free();
list_node_type *get_unused_vmm_node();
void print_all_free();
void print_all_used();
u32int *malloc(u32int size);
list_node_type *search_free(u32int size);
list_node_type *split_free(list_node_type *node, u32int size);
void free(u32int *virt_addr);
list_node_type *search_free_neighbor(list_node_type *node);
list_node_type *search_used(u32int virt_addr);
u32int *malloc_above(u32int size, u32int above, u32int align);
list_node_type *search_free_above(u32int size, u32int above);

#endif
