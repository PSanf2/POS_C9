#ifndef __TIMER_H
#define __TIMER_H

#include <system.h>

extern void timer_initialize(u32int freq);
extern void timer_interrupt_handler(__attribute__ ((unused)) registers regs);
extern u32int get_tick();

#endif
