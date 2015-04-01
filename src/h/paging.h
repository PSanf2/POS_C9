#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>

// typedef structs i'll be using
// these will make it easier to program things, and keep them straight
// the idea is that when i'm creating the page directories that instead
// of calculating everything on the fly, all willy-nilly, i'll actually
// have things organized neatly, and be able to quickly grab the needed
// information.

// KISS = Keep it simple, stupid!

/*
// i thought i would need this, but it's looking like i don't.
// it's not getting used for anything.
typedef struct page_table_struct
{
	u32int *virt_addr;
	u32int phys_addr;
} page_table_type;
*/

typedef struct page_directory_struct
{
	u32int *virt_addr;
	u32int phys_addr;
} page_directory_type;


// functions defined in the assembly file
extern u32int read_cr0();
extern void write_cr0(u32int);

extern u32int read_cr2();
extern void write_cr2(u32int);

extern u32int read_cr3();
extern void write_cr3(u32int);

extern u32int read_cr4();
extern void write_cr4(u32int);

// functions needed for paging stuff
void paging_initialize(struct multiboot *mboot_ptr);
void page_fault_interrupt_handler(registers regs);
void invlpg(u32int addr);

// stuff needed for the bitmap.
void set_frame(u32int frame_addr);
void clear_frame(u32int frame_addr);
u32int test_frame(u32int frame_addr);
u32int first_free();

// use to allocate a frame of physical memory, and return the physical address
u32int alloc_frame();

// will be use to switch the page directory so i can use another address space
void switch_page_directory(volatile page_directory_type *page_dir_ptr);

// will be used to map a physical address to a virtual address
void map_page(u32int virt_addr, u32int phys_addr);

// used to unmap a page
void unmap_page(u32int virt_addr);

void print_page_directory(volatile page_directory_type *page_directory);

#endif
