#ifndef __PMM_H
#define __PMM_H

#include <system.h>

void pmm_initialize(struct multiboot *mboot_ptr);
u32int alloc_frame();
void free_frame(u32int addr);

#endif
