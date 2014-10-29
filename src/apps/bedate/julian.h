
#ifndef _TIME_H
#include <time.h>
#endif

class juletime
{
	public :
	void getjuletime (int& juleday, int& juleyear);
	
	private :
	void systojule (int& juleday, int& juleyear);
	int exp (int number, int exponent);
};

