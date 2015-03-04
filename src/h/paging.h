#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>

// structs I want to use to make things easier
typedef struct memory_map_struct
{
	u32int addr;
	u32int length;
} __attribute__ ((packed)) memory_map;

// funtions i'll need in assembly

// functions i'll need to manipulate cr3 (read and write)
// functions i'll need to manipulate cr2 (read and write)
// functions i'll need to manipulate cr0 (read)

extern u32int read_cr0();
extern void write_cr0(u32int);

extern u32int read_cr2();

extern u32int read_cr3();
extern void write_cr3(u32int);

// paging stack initilzation function.
void paging_stack_initialize();

// functions i need to push and pop from the paging stack.
void paging_stack_push(u32int phys_addr);
u32int paging_stack_pop();
u32int paging_stack_full();
u32int paging_stack_empty();

// paging initialization function
void paging_initialize(struct multiboot *mboot_ptr);

// page fault handler
void page_fault_interrupt_handler(registers regs);

void invlpg(u32int addr);

#endif
