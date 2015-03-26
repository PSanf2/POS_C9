#include <string.h>

/*
 * A string is a char * that's cast to a string.
 * The string is an array of chars, with '\0' in the last element.
 * A string data type is defined in the string.h file
 */
s32int strcmp(const string str1, const string str2)
{
	s32int val = 0;
	s32int i = 0;
	while (str1[i] != '\0' || str2[i] != '\0')
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

u32int str_to_u32int(const string str)
{
	u32int result = 0;
	for (int i = 0; i < strlen(str); i++)
	{
		result = result * 10 + (str[i] - '0');
	}
	return result;
}

u32int hex_str_to_u32int(const string str)
{
	u32int result = 0;
	
	int i = 0;
	
	if ((strlen(str) > 2) && (str[0] == '0') && (str[1] == 'x'))
	{
		i = 2;	
	}
	
	for (; i < strlen(str); i++)
	{
		u32int digit = 0;
		char hexit = str[i];
		switch(hexit)
		{
			case 'A':
			case 'a':
				digit = 10;
				break;
			case 'B':
			case 'b':
				digit = 11;
				break;
			case 'C':
			case 'c':
				digit = 12;
				break;
			case 'D':
			case 'd':
				digit = 13;
				break;
			case 'E':
			case 'e':
				digit = 14;
				break;
			case 'F':
			case 'f':
				digit = 15;
				break;
			default:
				digit = str[i] - '0';
		}
		result = result * 16 + digit;
	}
	return result;
}





