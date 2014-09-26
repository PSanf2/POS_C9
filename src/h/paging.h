#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>

extern void paging_initialize();
extern void page_fault_set_handler(void (*callback)(u8int *buf, u16int size));
extern void page_fault_interrupt_handler(__attribute__ ((unused)) registers regs);

#endif
