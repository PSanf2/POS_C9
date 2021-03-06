#ifndef __STRING_H
#define __STRING_H

#include <system.h>

typedef char * string;

s32int strcmp(const string str1, const string str2);
u16int strlen(const string str);
string strcpy(string dest, const string source);
string strcat(string str1, const string str2);
u32int str_to_u32int(const string str);
u32int hex_str_to_u32int(const string str);

#endif
