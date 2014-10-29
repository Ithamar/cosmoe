
#ifndef _STRING_H
#include <string.h>
#endif

#ifndef _DISCORDIAN_H
#define _DISCORDIAN_H

const int STRING_SIZE = 90;		
typedef char str[STRING_SIZE];

class juletodis
{
	public :
	void todis (str outstr);
	
	private:
	void teens (int dayoseason, str outstr);
};
#endif

