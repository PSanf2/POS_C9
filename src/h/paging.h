#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>

// funtions i'll need in assembly

// functions i'll need to manipulate cr3 (read and write)
// functions i'll need to manipulate cr2 (read and write)
// functions i'll need to manipulate cr0 (read)

extern u32int read_cr0();
extern void write_cr0(u32int);

extern u32int read_cr2();

extern u32int read_cr3();
extern void write_cr3(u32int);

// functions i'll need in c

// paging initialization function
void paging_initialize();

// function to identity map a page table
void identity_map_page_table(u32int *my_page_directory, u32int table_index_in_directory);

// page fault handler
void page_fault_interrupt_handler(registers regs);



// function to create a page directory
// function to create a page table
// function i'll need to dynamically allocate a page (already on the physical memory manager)
// function i'll need to free a page (already on the physical memory manager)
// function i'll need to map a page?
// function i'll need to map a page table?

#endif
