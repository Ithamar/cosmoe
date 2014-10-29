#ifndef __COSMOE_IO_H__
#define __COSMOE_IO_H__



static inline void outb_p(unsigned char value, unsigned short port)
{
	__asm__ volatile
	(
#ifdef __i386__

		"outb %b0,%w1\n"
		"outb %%al,$0x80" : : "a" (value), "Nd" (port)
		
#elif defined(__ppc__)

		"stbx %0,0,%1\n"
		"1:	sync\n"
		"2:\n"
		".section __ex_table,\"a\"\n"
		"	.align	2\n"
		"	.long	1b,2b\n"
		".previous"
		: : "r" (value), "r" (port)

#endif
	);
} 


#endif /* ndef __COSMOE_IO_H__ */
