#ifndef __VMM_H
#define __VMM_H

#include <system.h>

void vmm_initialize(u32int bitmap_addr);
void set_page(u32int virt_addr);
void clear_page(u32int virt_addr);
u32int test_page(u32int virt_addr);
u32int *kmalloc(u32int num_pages);
void kfree(u32int *virt_addr);

#endif
