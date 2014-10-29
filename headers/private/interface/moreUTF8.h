#ifndef __MOREUTF8
#define __MOREUTF8


inline char *
next_utf8(char *string)
{
	char *ptr = string;
		
	do {
		if (ptr == 0) 	//If we are at the end of the string,
			break;		//bail out
		ptr++;
	} while ((*ptr & 0xc0) == 0x80);
	
	return ptr;
}


inline char *
previous_utf8(char *string, char *position)
{
	char *ptr = position;
	
	do {
		if (ptr == string) 	//If we are at the beginning of the string,
			break;			//bail out
		ptr--;
	} while ((*ptr & 0xc0) == 0x80);
	
	return ptr;
}


#endif
