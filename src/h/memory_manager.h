#ifndef __MEMORY_MANAGER_H
#define __MEMORY_MANAGER_H

#include <system.h>

typedef struct memory_map_struct
{
	u32int addr;
	u32int length;
} __attribute__ ((packed)) memory_map;

extern void push_physical_address(u32int addr);
extern u32int pop_physical_address();
extern void free_block(u32int addr);
extern u32int allocate_block();
extern void memory_manager_initialize(struct multiboot *mboot_ptr);

#endif
