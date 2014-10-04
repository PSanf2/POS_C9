#ifndef __STRING_H
#define __STRING_H

#include <system.h>

typedef char * string;

extern s32int strcmp(const string str1, const string str2);
extern u16int strlen(const string str);
extern string strcpy(string dest, const string source);
extern string strcat(string str1, const string str2);
extern u32int str_to_u32int(const string str);

#endif
