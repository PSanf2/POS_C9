#include <memory.h>
#include <vga.h>
 
void memcpy(u8int *dest, const u8int *src, u32int len)
{
	const u8int *sp = (const u8int *) src;
	u8int *dp = (u8int *) dest;
	for (; len != 0; len--)
		*dp++ = *sp++;
}

void memset(u8int *dest, u8int val, u32int len)
{
	u8int *temp = (u8int *) dest;
	for (; len != 0; len--)
		*temp++ = val;
}
