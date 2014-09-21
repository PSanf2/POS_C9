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
	int i = 0;
	while (str1[i] != '\0')
	{
		if (str2[i] == '\0')
		{
			val = -1;
			break;
		}
		
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