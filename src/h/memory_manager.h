#ifndef __MEMORY_MANAGER_H
#define __MEMORY_MANAGER_H

#include <multiboot.h>
#include <system.h>

typedef struct memory_map_struct
{
	u32int addr;
	u32int length;
} __attribute__ ((packed)) memory_map;

extern void memory_manager_initialize(struct multiboot *mboot_ptr, memory_map mem_map);

#endif
