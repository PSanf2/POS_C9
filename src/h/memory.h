#ifndef __MEMORY_H
#define __MEMORY_H

#include <multiboot.h>
#include <system.h>

typedef struct node_struct
{
	struct node_struct *prev;
	u32int *data;
	u32int *size;
	struct node_struct *next;
} node;

typedef struct list_struct
{
	struct node_struct *first;
	struct node_struct *last;
} list;

extern void memcpy(u8int *dest, const u8int *src, u32int len);
extern void memset(u8int *dest, u8int val, u32int len);

// i need to set up two lists. one for the free memory, one for the allocated memory
// i'll need to have wrapper functions to manage the lists
// i need to have a function to find a free block of memory large enough to fulfill an allocation request
// i need to have a function to split a block of free memory into two blocks of arbitrary size
	// "take this big block of memory, and spit it in two at this offset"
// i need to have a function to identify adjacent free blocks of memory
// i need to have a function to merge two free blocks of memory
// i need to have a function to initialize the memory manager
// i need to have a function to allocate n bytes of memory
// i need to have a function to free an allocated block at a specified address

void memory_manager_initialize(struct multiboot *mboot_ptr);
u32int *malloc(u32int bytes);
u32int *malloc_align(u32int bytes, u32int align);
void free(u32int *addr);
void print_mem_state();
void print_node_state(u32int *myNodeAddr);

#endif
