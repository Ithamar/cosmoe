/************************************************************************************************
This class takes no input and outputs 2 ints through the public accessor function getjultime.
Reason for no input is because it is not needed - input is taken fron the BIOS through the 
<time.h> function.  
*************************************************************************************************/

#ifndef JULIAN_H
#include "julian.h"
#endif

void juletime::systojule (int & juleday, int & juleyear)
{
	long now=0 ,year=0;				//Julian calender // Was  u long
	struct tm *pTime;
	char *c, buffer [5] = "0000";
	int i,j=0, test;					//counters, test to see if number is 0
	
	time (&now);						// record current time
	pTime = localtime (&now);			// 
	c=asctime(pTime);
	strftime(buffer,5,"%j",pTime);			// Put Julian days in buffer 
	for (i = 4 ; i > -1 ; i--) 			// Get Julian day into int julday
	{
		test = int (buffer[i]);
		if (test != 0) 
		{
			juleday += int ((buffer[i]-48)*(exp (10,j)));
			j++;
		}
	}									// this is needed because I need to play with ints
	
	strftime(buffer,5,"%Y",pTime);		// Put year in buffer
	j=0;
	for (i = 4; i > -1; i--)			//Get bios year into year
	{
		test = int (buffer[i]);
		if (test !=0)
		{
			year += int (buffer[i]-48)*(exp (10,j));
			j++;                      // Builds the year by exponentiating the digits
		}
	}
	juleyear = year;			// translate year to dis year
}

void juletime::getjuletime (int & juleday, int & juleyear)			// public accessor
{
	
	systojule (juleday,juleyear);
}	

int juletime:: exp (int number, int exponent)		// a simple exponentiator
/* this was written to avoid including math.h.  Seemed like a lot to add
for a relatively simple expression*/ 
{
	int result = number;
	if (exponent == 0) return 1;
	if (exponent == 1) return number;
	while (exponent > 1)
	{
		result *= number;
		exponent --;
	}
	return result;
}

