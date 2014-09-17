#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include <system.h>

#define KEYBOARD_BUFFER_SIZE 4096

extern void keyboard_flush();
extern void keyboard_set_handler(void (*callback)(u8int *buf, u16int size));
extern void keyboard_initialize();
extern void keyboard_interrupt_handler(__attribute__ ((unused)) registers regs);

#endif
