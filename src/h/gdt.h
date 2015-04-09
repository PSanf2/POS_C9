#ifndef __GDT_H
#define __GDT_H

#include <system.h>

extern void gdt_flush(u32int);

struct gdt_entry
{
	u16int limit_low;
	u16int base_low;
	u8int  base_middle;
	u8int  access;
	u8int  granularity;
	u8int  base_high;
} __attribute__((packed));

struct gdt_ptr
{
	u16int limit;
	u32int base;
} __attribute__ ((__packed__));

struct gdt_entry gdt[3];
struct gdt_ptr gdtptr;

void gdt_initialize();
void gdt_set_gate(s32int num, u32int base, u32int limit, u8int access, u8int gran);

#endif
