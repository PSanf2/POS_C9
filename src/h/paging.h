#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>

// data structures and type definitions

typedef struct page_table_struct
{
	u32int *virt_addr;
	u32int phys_addr;
} page_table_type;

typedef struct page_directory_struct
{
	u32int *virt_addr;
	u32int phys_addr;
	page_table_type tables[1024];
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

// function included in the c file
void paging_initialize();
void page_fault_interrupt_handler(registers regs);
void invlpg(u32int addr);
u32int virt_to_phys(page_directory_type *page_directory, u32int virt_addr);
void map_page(u32int virt_addr, u32int phys_addr);
void unmap_page(u32int virt_addr);
void change_page_directory(page_directory_type *page_directory);
u32int get_table_attribs(u32int page_dir_index);
void copy_page_directory(page_directory_type *source, page_directory_type *dest);
void copy_page_table(page_table_type *source, page_table_type *dest);

#endif
