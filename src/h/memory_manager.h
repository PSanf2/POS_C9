#ifndef __MEMORY_MANAGER_H
#define __MEMORY_MANAGER_H

#include <system.h>

typedef struct memory_map_struct
{
	u32int addr;
	u32int length;
} __attribute__ ((packed)) memory_map;

extern void push_physical_page(u32int addr);
extern u32int pop_physical_page();
extern u32int map_page(u32int virt_addr, u32int phys_addr);
extern u32int map_pages(u32int virt_addr, u32int phys_addr, u32int count);
extern void switch_page_directory(u32int *addr);
extern u32int *create_page_table(u32int count);
extern u32int alloc_pages(u32int *start_at, u32int count);
extern u32int free_pages(u32int *start_at);
extern void memory_manager_initialize(struct multiboot *mboot_ptr, memory_map mem_map);

#endif
