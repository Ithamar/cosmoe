/*  libcosmoe.so - the interface to the Cosmoe UI
 *  Portions Copyright (C) 2001-2002 Bill Hayden
 *  Portions Copyright (C) 1999-2001 Kurt Skauen
 *  Portions Copyright (C) 2001 Tom Marshall
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */


#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/uio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <linux/kd.h>

#include "../config.h"
#include <Debug.h>
#include <SupportDefs.h>
#include <fs_attr.h>

#define DEBUG_IO

#ifdef DEBUG_IO
#include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#define KBD_TTY_OPENED     0x0001
#define KBD_TERMIO_CHANGED 0x0002
#define KBD_MODE_CHANGED   0x0004

uint32         g_kbd_state = 0;
int            g_tty_fd = -1;
struct termios g_tiof_orig;
int            g_kdf_orig;

/*
 * http://linuxassembly.org/rawkb.html
 */
int restore_keyboard( void )
{
#if defined(COSMOE_SDL)
	return 0;
#endif

	if( g_kbd_state & KBD_TERMIO_CHANGED )
	{
		ioctl( g_tty_fd, TCSETSW, &g_tiof_orig );
	}

	if( g_kbd_state & KBD_MODE_CHANGED )
	{
#if !defined(COSMOE_DIRECTFB) && !defined(COSMOE_SDL)
		ioctl( g_tty_fd, KDSKBMODE, g_kdf_orig );
#endif
	}

	if( g_kbd_state & KBD_TTY_OPENED )
	{
#if !defined(COSMOE_DIRECTFB) && !defined(COSMOE_SDL)

		/* turn on cursor */
		write( g_tty_fd, "\x1b[?25h", 6 );
		/* enable blanking, 300s (how to get the old value?) */
		write( g_tty_fd, "\x1b[9;300]", 8 );
		/* move cursor to (1,1) and clear screen */
		write( g_tty_fd, "\x1b[1;1H" "\x1b[2J", 6+4 );
#endif
		close( g_tty_fd );
	}

	g_kbd_state = 0;

	return 0;
}


int init_keyboard( void )
{
	int            res;
	int            tmp;
	int            kdf_new;
	struct termios tiof_new;

	if( g_kbd_state != 0 )
	{
		STRACE( "linux_compat: init_keyboard already called\n" );
		return -1;
	}

	/* make sure we are on console and /dev/tty is available */
	tmp = 12; /* get-fg-console */
#if 0
	/* XXX: this fails when the server is backgrounded (eg. "appserver &") */
	res = ioctl( STDIN_FILENO, TIOCLINUX, &tmp );
	if( res < 0 )
	{
		STRACE( "linux_compat: init_keyboard STDIN check failed: %s\n", strerror(errno) );
		return -1;
	}
	res = ioctl( STDOUT_FILENO, TIOCLINUX, &tmp );
	if( res < 0 )
	{
		STRACE( "linux_compat: init_keyboard STDOUT check failed: %s\n", strerror(errno) );
		return -1;
	}
#endif

	g_tty_fd = open( "/dev/tty", O_RDWR );
	if( g_tty_fd < 0 )
	{
		STRACE( "linux_compat: init_keyboard console check failed (/dev/tty: %s)\n", strerror(errno) );
		return -EPERM;
	}
	g_kbd_state |= KBD_TTY_OPENED;
	STRACE( "linux_compat: keyboard fd = %d\n", g_tty_fd );

	/* get current console and keyboard settings */

	/* XXX: to prevent console switching, use VT_GETMODE and set mode to VT_PROCESS */

	res = ioctl( g_tty_fd, TCGETS, &g_tiof_orig );
	if( res < 0 )
	{
		STRACE( "linux_compat: init_keyboard cannot get termio settings\n" );
		return -1;
	}

#if !defined(COSMOE_DIRECTFB) && !defined(COSMOE_SDL)
	res = ioctl( g_tty_fd, KDGKBMODE, &g_kdf_orig );
	if( res < 0 )
	{
		STRACE( "linux_compat: init_keyboard cannot get keyboard settings (%d)\n", res );
		return -1;
	}

	/* move cursor to (1,1) and clear screen */
	write( g_tty_fd, "\x1b[1;1H" "\x1b[2J", 6+4 );
	/* disable blanking */
	write( g_tty_fd, "\x1b[9;0]", 6 );
	/* turn off cursor */
	write( g_tty_fd, "\x1b[?25l", 6 );
#endif
	tiof_new = g_tiof_orig;
	tiof_new.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IXON | IXOFF);
	tiof_new.c_lflag &= ~(ECHO | ICANON | ISIG);
	kdf_new = K_RAW;

	res = ioctl( g_tty_fd, TCSETSW, &tiof_new );
	if( res < 0 )
	{
		STRACE( "linux_compat: init_keyboard cannot set termio settings fd=%d, (%s)\n",
				g_tty_fd, strerror(errno) );
		return -1;
	}
	g_kbd_state |= KBD_TERMIO_CHANGED;

#if !defined(COSMOE_DIRECTFB) && !defined(COSMOE_SDL)
	res = ioctl( g_tty_fd, KDSKBMODE, kdf_new );
	if( res < 0 )
	{
		STRACE( "linux_compat: init_keyboard cannot set keyboard settings\n" );
		return -1;
	}
	g_kbd_state |= KBD_MODE_CHANGED;
#endif

	return g_tty_fd;
}
