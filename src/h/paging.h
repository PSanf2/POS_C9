#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>

extern u32int read_cr0();
extern void write_cr0(u32int);

extern u32int read_cr2();
extern void write_cr2(u32int);

extern u32int read_cr3();
extern void write_cr3(u32int);

extern u32int read_cr4();
extern void write_cr4(u32int);

void paging_print_info();

// paging initialization function
void paging_initialize(struct multiboot *mboot_ptr);

// page fault handler
void page_fault_interrupt_handler(registers regs);

void invlpg(u32int addr);



#endif
