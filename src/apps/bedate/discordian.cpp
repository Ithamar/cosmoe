/**************************************************************************************************
This object returns a string based on integers defined in the object.  This makes it somewhat 
useless to other programmers - a good object should have clear inputs and outputs.  As this is
among my first objects I won't spend much time worrying about it.  
*****************************************************************************************************/ 
#ifndef _DISCORDIAN_H
#include "discordian.h"
#endif

#ifndef JULIAN_H
#include "julian.h"
#endif

#include <String.h>

void juletodis::todis (str outstr)
{	
	bool tibsday = false;						// In the discordian calender there is a day
												// that corresponds to feb 29 - St Tibbs Day
	int dayoseason ,dayoweek , intseason , tens, ones;
	int julday =0, disyear=0;					
	int tempyear,i,j,place;
	juletime thetime;							// Juletime is a public accessor that returns the 
												// day and year from the BIOS through the <time.h> 
												// function
	
	
	thetime.getjuletime(julday, disyear);		
	disyear += 1166;							// Discordian year is 1166 years ahead of ours.  
	if (disyear%4 == 2) tibsday = true;			// test for "leap year"
	
	if (tibsday && (julday > 59)) 				// Account for Tib's day if it is present
	{
		julday--;
		if (julday==59) 						// test for Tib's day
		{
			strcat(outstr,"It is St. Tib's day, ");
			strcat (outstr, "in the year of ");
			for (i=0; i<4; i++)					// make the year string and cat it onto outstr 1 digit at a time
			{
				place = 1;
				for (j=i; j<3; j++){place *=10;}
				tempyear = (disyear/place);
				switch (tempyear)
				{
					case 1:
						strcat ( outstr, "1");
						break;
					case 2:
						strcat ( outstr, "2");
						break;
					case 3:
						strcat ( outstr, "3");
						break;
					case 4:
						strcat ( outstr, "4");
						break;
					case 5:
						strcat ( outstr, "5");
						break;
					case 6:
						strcat ( outstr, "6");
						break;
					case 7:
						strcat ( outstr, "7");
						break;
					case 8:
						strcat ( outstr, "8");
						break;
					case 9:
						strcat ( outstr, "9");
						break;
					default:
						break;
				}
			disyear%=place;
			}
			return;
		}
		
	}														// End If for Tibbs Day - this could have 
															// been a seperate function, but it wasn't
															// A better Idea would be to make everything
															//from the last comment a function - it is
															// used again later
	dayoweek = julday % 5;
	strcat(outstr, "Today is ");
	switch (dayoweek)										// Discordian week has 5 days...
	{
		case 0:	strcat ( outstr, "Sweetmorn ");
				break;
		case 1: strcat ( outstr, "Boomtime ");
				break;
		case 2:	strcat ( outstr, "Pungenday ");
				break;
		case 3: strcat ( outstr, "Prickle-Pickle ");
				break;
		case 4: strcat ( outstr, "Setting Orange ");
				break;
		default : strcat ( outstr, "Application broken!!! shoot programmer!");	
				break;										// This is a a "funny" 
															// way to say I (the programmer)
															// screwed up
	}
	
	dayoseason = (julday%73);								// There are 5 seasons in the year - 
															// Like the 12 months in the Julian
	if (dayoseason == 0) dayoseason = 73; 
	tens = dayoseason /10;									// How many tens? (like 1st grade again)
	ones = dayoseason % 10;									// How many ones?
	switch (tens)
	{														// Cat tens on - there is a special case
															// for Teens below
		case 0: 
			break;
		case 2:
			if (ones == 0) strcat ( outstr,"the Twentieth day of ");
			else strcat ( outstr, "the Twenty-");
			break;
		case 3:
			if (ones == 0) strcat ( outstr,"the Thirtieth day of ");
			else strcat ( outstr, "the Thirty-");
			break;
		case 4:
			if (ones == 0) strcat ( outstr,"the Fortieth day of ");
			else strcat ( outstr, "the Fourty-");
			break;
		case 5:
			if (ones == 0) strcat ( outstr,"the Fiftieth day of ");
			else strcat ( outstr, "the Fifty-");
			break;
		case 6:
			if (ones == 0) strcat ( outstr,"the Sixtieth day of ");
			else strcat ( outstr, "the Sixty-");
			break;
		case 7:
			if (ones == 0) strcat ( outstr,"the Seventieth day of ");
			else strcat ( outstr, "the Seventy-");
			break;
		default:
			teens(dayoseason, outstr);								// special case for teens
			break;
	}
	if (tens !=1)													// Check for special teens case
	{
		switch (ones)
		{
			case 1:
				strcat ( outstr, "First day of ");
				break;
			case 2:
				strcat ( outstr, "Second day of ");
				break;
			case 3:
				strcat ( outstr, "Third day of ");
				break;
			case 4:
				strcat ( outstr, "Fourth day of ");
				break;
			case 5:
				strcat ( outstr, "Fifth day of ");
				break;
			case 6:
				strcat ( outstr, "Sixth day of ");
				break;
			case 7:
				strcat ( outstr, "Seventh day of ");
				break;
			case 8:
				strcat ( outstr, "Eigth day of ");
				break;
			case 9:
				strcat ( outstr, "Ninth day of ");
				break;
		}
	}
	intseason = (julday/73);
	switch (intseason)													// Which season (like a month 
																		// with 73 days
	{
		case 0: strcat ( outstr,  "Chaos ");
			break;
		case 1: strcat ( outstr,  "Dischord ");
			break;
		case 2: strcat ( outstr,  "Confusion ");
			break;
		case 3: strcat ( outstr,  "Bureaucracy ");
			break;	
		default : strcat ( outstr,  "The Aftermath ");
	}
	strcat (outstr, "in the year of ");
	for (i=0; i<4; i++)
	{
		place = 1;
		for (j=i; j<3; j++){place *=10;}								// here's that icky code again.
		tempyear = (disyear/place);
		switch (tempyear)
		{
			case 1:
				strcat ( outstr, "1");
				break;
			case 2:
				strcat ( outstr, "2");
				break;
			case 3:
				strcat ( outstr, "3");
				break;
			case 4:
				strcat ( outstr, "4");
				break;
			case 5:
				strcat ( outstr, "5");
				break;
			case 6:
				strcat ( outstr, "6");
				break;
			case 7:
				strcat ( outstr, "7");
				break;
			case 8:
				strcat ( outstr, "8");
				break;
			case 9:
				strcat ( outstr, "9");
				break;
			default:
				break;
		}
	disyear %= place;
	}
}

void juletodis::teens (int dayoseason, str outstr)					//special case for teens
{
	switch (dayoseason)
	{
		case 10:
			strcat ( outstr, "Tenth day of ");
			break;
		case 11:
			strcat ( outstr, "Eleventh day of ");
			break;
		case 12:
			strcat ( outstr, "Twelth day of ");
			break;
		case 13:
			strcat ( outstr, "Thirteenth day of ");
			break;
		case 14:
			strcat ( outstr, "Fourteenth day of ");
			break;
		case 15:
			strcat ( outstr, "Fifteenth day of ");
			break;
		case 16:
			strcat ( outstr, "Sixteenth day of ");
			break;
		case 17:
			strcat ( outstr, "Seventeenth day of ");
			break;
		case 18:
			strcat ( outstr, "Eithteenth day of ");
			break;
		case 19:
			strcat ( outstr, "Ninteenth day of ");
			break;
	}
}

	
