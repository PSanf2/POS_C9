#ifndef __PORT_H
#define __PORT_H

#include <system.h>

void outb(u16int port, u8int value);
u8int inb(u16int port);

#endif
