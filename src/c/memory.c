#include <memory.h>
 
void memcpy(u8int *dest, const u8int *src, u32int len)
{
	const u8int *sp = (const u8int *) src;
	u8int *dp = (u8int *) dest;
	while (len--)
	{
		*dp++ = *sp++;
	}
}

void memset(u8int *dest, u8int val, u32int len)
{
	u8int *temp = (u8int *) dest;
	while (len--)
	{
		*temp++ = val;
	}
}

void memmove(u8int *dest, u8int *src, u32int len)
{
	u8int *sp = (u8int *) src;
	u8int *dp = (u8int *) dest;
	if (dp < sp)
	{
		while (len--)
		{
			*dp++ = *sp++;
		}
	}
	else
	{
		u8int *lasts = sp + (len - 1);
		u8int *lastd = dp + (len - 1);
		while (len--)
		{
			*lastd-- = *lasts--;
		}
	}
}

int memcmp(const u8int *src1, const u8int *src2, u32int len)
{
	const u8int *s1 = (const u8int *) src1;
	const u8int *s2 = (const u8int *) src2;
	while (len--)
	{
		if (*s1++ != *s2++)
		{
			return s1[-1] < s2[-1] ? -1 : 1;
		}
	}
	return 0;
}
