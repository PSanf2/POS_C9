#ifndef __MEMORY_H
#define __MEMORY_H

#include <system.h>

extern void memcpy(u8int *dest, const u8int *src, u32int len);
extern void memset(u8int *dest, u8int val, u32int len);
extern void memmove(u8int *dest, u8int *src, u32int len);
extern int memcmp(const u8int *src1, const u8int *src2, u32int len);

#endif
