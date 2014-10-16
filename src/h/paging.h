#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>

extern u32int read_cr0();
extern void write_cr0(u32int);
extern u32int read_cr3();
extern void write_cr3(u32int);

extern void paging_initialize();
extern void page_fault_set_handler(void (*callback)(u8int *buf, u16int size));
extern void page_fault_interrupt_handler(__attribute__ ((unused)) registers regs);

#endif
