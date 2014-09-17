#ifndef __PORT_H
#define __PORT_H

#include <system.h>

extern void outb(u16int port, u8int value);
extern u8int inb(u16int port);

#endif
