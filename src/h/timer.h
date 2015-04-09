#ifndef __TIMER_H
#define __TIMER_H

#include <system.h>

void timer_initialize(u32int freq);
void timer_interrupt_handler(__attribute__ ((unused)) registers regs);
u32int get_tick();

#endif
