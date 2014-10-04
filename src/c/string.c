#include <string.h>

/*
 * A string is a char * that's cast to a string.
 * The string is an array of chars, with '\0' in the last element.
 * A string data type is defined in the string.h file
 */
 
// this is used to compare two strings
s32int strcmp(const string str1, const string str2)
{
	s32int val = 0;
	s32int i = 0;
	while (str1[i] != '\0')
	{
		if (str1[i] > str2[i])
		{
			val = 1;
			break;
		}
		else if (str1[i] < str2[i])
		{
			val = -1;
			break;
		}
		i++;
	}
	return val;
}

u16int strlen(const string str)
{
	int i = 0;
	while (str[i] != '\0')
	{
		i++;
	}
	return i;
}

string strcpy(string dest, const string source)
{
	int i = 0;
	while(source[i] != '\0') {
		dest[i] = source[i];
		i++;
	}
	dest[i+1] = '\0';
	return dest;
}

string strcat(string str1, const string str2)
{
	strcpy(str1 + strlen(str1), str2);
	return str1;
}

extern u32int str_to_u32int(const string str)
{
	u32int result = 0;
	for (int i = 0; i < strlen(str); i++)
	{
		result = result * 10 + (str[i] - '0');
	}
	return result;
}
