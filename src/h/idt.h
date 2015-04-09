#ifndef __IDT_H
#define __IDT_H

#include <system.h>

extern void idt_flush(u32int);

struct idt_entry
{
	u16int base_lo;
	u16int sel;
	u8int always0;
	u8int flags;
	u16int base_hi;
} __attribute__((packed));

struct idt_ptr
{
	u16int limit;
	u32int base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtptr;

void idt_initialize();
void idt_set_gate(u8int num, u32int base, u16int sel, u8int flags);

#endif
