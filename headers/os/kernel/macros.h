#ifndef _MACROS_H_
#define _MACROS_H_

#define	ABS(a)	(((a)>=0) ? (a) : (-(a)))

#define	min(a,b)	(((a)<(b)) ? (a) : (b) )
#define	max(a,b)	(((a)>(b)) ? (a) : (b) )

#define	__assertw( expr ) do {if ( !(expr) ) 							\
  dbprintf( "Assertion failure (" #expr ")\n"							\
	  "file: " __FILE__ " function: %s() line: %d\n%p\n%p\n%p\n%p\n",			\
	  __FUNCTION__, __LINE__, __builtin_return_address(0), __builtin_return_address(1),	\
	  __builtin_return_address(2), __builtin_return_address(3)  ); } while(0)

#endif
