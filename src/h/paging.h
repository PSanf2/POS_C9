#ifndef __PAGING_H
#define __PAGING_H

#include <system.h>
#include <isr.h>
#include <memory.h>

struct page_struct
{
    u32int present    : 1;   // Page present in memory
    u32int rw         : 1;   // Read-only if clear, readwrite if set
    u32int user       : 1;   // Supervisor level only if clear
    u32int accessed   : 1;   // Has the page been accessed since last refresh?
    u32int dirty      : 1;   // Has the page been written to since last refresh?
    u32int unused     : 7;   // Amalgamation of unused and reserved bits
    u32int frame      : 20;  // Frame address (shifted right 12 bits)
} __attribute__((packed));

typedef struct page_struct page;

struct page_table_struct
{
	page pages[1024];
} __attribute__((packed));

typedef struct page_table_struct page_table;

struct page_directory_struct
{
	page_table *tables[1024];
	u32int tablesPhysical[1024];
	u32int physicalAddr;
} __attribute__((packed));

typedef struct page_directory_struct page_directory;

extern void paging_initialize();
extern u32int paging_malloc_int(u32int size, int align, u32int *phys);
extern void alloc_frame(page *my_page, int is_kernel, int is_writeable);
extern page *get_page(u32int address, int make, page_directory *dir);
extern void switch_page_directory(page_directory *dir);
extern void page_fault_interrupt_handler(__attribute__ ((unused)) registers regs);

#endif
