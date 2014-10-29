//------------------------------------------------------------------------------
//	Copyright (c) 2003, Tom Marshall
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		misc.cpp
//	Authors:		Tom Marshall (tommy@tig-grr.com)
//------------------------------------------------------------------------------


#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <errno.h>
#include <unistd.h>

#include <Debug.h>
#include <SupportDefs.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(linux)
#include <sys/sysinfo.h>
#else
#warning System information not available on this platform
#warning system_time() will always return 0 on this platform
#endif

int dbprintf( const char* pzFmt, ... )
{
	int rc;
	va_list ap;

	va_start( ap, pzFmt );
	rc = vfprintf( stderr, pzFmt, ap );
	va_end(ap);

	return rc;
}





/*
 * return usecs idle since boot: this is the last field in the main cpu
 * entry in /proc/stat, but that's in jiffies.  scaling factor is wired
 * to HZ=100 here, which is not always correct.  note that the logic is
 * complicated by the fact that uniprocessor /proc/stat does not have a
 * "cpu0" entry.
 */
bigtime_t get_idle_time( int nProcessor )
{
	FILE*         fp;
	char          cpuid[32];
	char          buf[80];
	unsigned long n1, n2, n3, nidle, nidle_all;
	bigtime_t     idletime;

	sprintf( cpuid, "cpu%d ", nProcessor );
	nidle = 0;
	nidle_all = 0;
	idletime = 0;
	if( (fp = fopen( "/proc/stat", "r" )) != NULL )
	{
		while( fgets( buf, sizeof(buf), fp ) != NULL )
		{
			if( strncmp( buf, "cpu ", 4 ) == 0 )
			{
				sscanf( buf+4, "%lu %lu %lu %lu", &n1, &n2, &n3, &nidle_all );
			}
			if( strncmp( buf, cpuid, strlen(cpuid) ) == 0 )
			{
				sscanf( buf+strlen(cpuid), "%lu %lu %lu %lu", &n1, &n2, &n3, &nidle );
				break;
			}
		}
		fclose( fp );
	}

	if( nProcessor == 0 && nidle == 0 )
	{
		/* use the "cpu" line */
		idletime = (bigtime_t)nidle_all * 10000LL;
	}
	else
	{
		/* use the "cpuN" line */
		idletime = (bigtime_t)nidle * 10000LL;
	}

	return idletime;
}


/* helper for get_system_info */
static void get_cpu_info( system_info* psInfo )
{
#if defined(linux)
	FILE*         fp;
	int           ncpu;
	char          buf[80];
	char*         p;
	bigtime_t     systime;
	bigtime_t     idletime;
	unsigned long n1, n2, n3, nidle;

	systime = system_time();
	psInfo->boot_time = real_time_clock_usecs() - systime;
	psInfo->bus_clock_speed = 66;	/* FIXME */
	ncpu = 0;
	if( (fp = fopen( "/proc/cpuinfo", "r" )) != NULL )
	{
		while( fgets( buf, sizeof(buf), fp ) != NULL )
		{
			if( strncmp( buf, "processor\t", 10 ) == 0 )
			{
				ncpu++;
			}

			if( strncmp( buf, "cpu MHz\t", 8 ) == 0 &&
				ncpu < B_MAX_CPU_COUNT )
			{
				p = strchr( buf, ':' );
				if( p != NULL )
				{
					psInfo->cpu_clock_speed = atoi( p+2 );
				}
			}
		}
		fclose( fp );
	}
	psInfo->cpu_count = ncpu;

	if( (fp = fopen( "/proc/stat", "r" )) != NULL )
	{
		while( fgets( buf, sizeof(buf), fp ) != NULL )
		{
			if( ncpu == 1 && strncmp( buf, "cpu ", 4 ) == 0 )
			{
				/* there are no cpuN lines, use the overall stat */
				sscanf( buf+4, "%lu %lu %lu %lu", &n1, &n2, &n3, &nidle );
				idletime = (bigtime_t)nidle * 10000LL;
				psInfo->cpu_infos[0].active_time = systime - idletime;
				break;
			}

			if( strncmp( buf, "cpu", 3 ) == 0 )
			{
				sscanf( buf+3, "%d %lu %lu %lu %lu", &ncpu, &n1, &n2, &n3, &nidle );
				if( ncpu < psInfo->cpu_count )
				{
					idletime = (bigtime_t)nidle * 10000LL;
					psInfo->cpu_infos[ncpu].active_time = systime - idletime;
				}
			}
		}
		fclose( fp );
	}
#endif
}

/* helper for get_system_info */
static void get_mem_info( system_info* psInfo )
{
}


/* helper for get_system_info */
static void get_fs_info( system_info* psInfo )
{
}


status_t _get_system_info( system_info* psInfo, size_t size )
{
	struct utsname unamebuffer;

	if (uname(&unamebuffer) == 0)
	{
		strcpy( psInfo->kernel_name, unamebuffer.sysname );
		strcpy( psInfo->kernel_build_date, unamebuffer.release );
		strcpy( psInfo->kernel_build_time, "unknown" );
	}
	else
	{
		strcpy( psInfo->kernel_name, "unknown" );
		strcpy( psInfo->kernel_build_date, "unknown" );
		strcpy( psInfo->kernel_build_time, "unknown" );
	}
	psInfo->kernel_version = 2LL;
	get_cpu_info( psInfo ); /* set boot time and cpu info */
	get_mem_info( psInfo ); /* set various mem info */
	get_fs_info( psInfo );  /* set various fs info */

	return 0;
}


int32	is_computer_on(void)
{
	return 1L;
}


double	is_computer_on_fire(void)
{
	return 3.1415926536;
}


void	debugger(const char *message)
{
	printf("BUG: %s\n", message);
}

/* return usecs since boot */
bigtime_t
system_time(void)
{
	FILE*       fp;
	char        buf[80];
	double      utime;
	bigtime_t   systime;

	systime = 0;
	if( (fp = fopen( "/proc/uptime", "r" )) != NULL )
	{
		if( fgets( buf, sizeof(buf), fp ) != NULL )
		{
			sscanf( buf, "%lf", &utime );
			systime = (bigtime_t)( utime * 1000000.0f );
		}
		fclose( fp );
	}

	return systime;
}

int watch_node( int nNode, int nPort, uint32 nFlags, void* pDate )
{
	dbprintf( "Cosmoe: UNIMPLEMENTED: watch_node\n" );
	return -1;
}


