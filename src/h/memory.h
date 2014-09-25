#ifndef __MEMORY_H
#define __MEMORY_H

#include <system.h>

extern void memcpy(u8int *dest, const u8int *src, u32int len);
extern void memset(u8int *dest, u8int val, u32int len);
extern void memset32(u32int *dest, u32int val, u32int len);

#endif
