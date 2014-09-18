#ifndef __SYSTEM_H
#define __SYSTEM_H

#ifndef NULL
#define NULL 0
#endif

typedef unsigned int   u32int;
typedef          int   s32int;
typedef unsigned short u16int;
typedef          short s16int;
typedef unsigned char  u8int;
typedef          char  s8int;

// define a structure representing the system registers.
struct registers_struct
{
    u32int ds;
    u32int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32int int_no, err_code;
    u32int eip, cs, eflags, useresp, ss;
};

// define the structure to be a data type
typedef struct registers_struct registers;

// define a datatype for a function that has one paramater of type registers and returns a *isr (function pointer)[i think]
typedef void (*isr)(registers);

#define enable_interrupts() asm volatile("sti")
#define disable_interrupts() asm volatile("cli")

#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <irq.h>
#include <keyboard.h>
#include <memory.h>
#include <port.h>
#include <timer.h>
#include <vga.h>
#include <keyboard.h>

void kernel_keyboard_handler(u8int *buf, u16int size);
void kernel_vga_handler(u8int *buf, u16int size);

#endif
