#ifndef __VMM_H
#define __VMM_H

#include <system.h>

typedef struct list_struct list_type;
typedef struct list_node_struct list_node_type;

typedef struct vmm_data_struct
{
	u32int virt_addr;
	u32int size;
} vmm_data_type;

void vmm_initialize();
u32int *malloc(u32int size);
u32int *malloc_align(u32int size, u32int align);
u32int *malloc_above(u32int size, u32int align, u32int above);
void free(u32int *virt_addr);
void vmm_print_node(list_node_type *node);
void vmm_print_list(list_type *list);
void vmm_print_free();
void vmm_print_used();
list_node_type *split_free(list_node_type *node, u32int size);
list_node_type *search_free(u32int size, u32int above);
list_node_type *get_unused_node();

list_node_type *search_used(u32int *virt_addr);
list_node_type *search_free_neighbor(list_node_type *node);
list_node_type *search_adjacent_free();
void compact_after(list_node_type *node);
void compact_all_free();

#endif
