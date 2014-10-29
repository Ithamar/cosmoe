/*
 *  cterm - x-term like terminal emulator for Cosmoe
 *  Copyright (C) 1999  Kurt Skauen
 *  Copyright (C) 2002-2003  Bill Hayden
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>

#ifdef __APPLE__
#include <util.h>
#else
#include <getopt.h>
#endif

#include <pwd.h>

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <OS.h>
#include <kernel.h>

#include <Window.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <Application.h>
#include <Message.h>
#include "tview.h"

enum
{
    ID_SCROLL,
    ID_REFRESH
};

int  g_nShowHelp = 0;
int  g_nShowVersion = 0;
int  g_nDebugLevel = 0;
bool g_bWindowBorder = true;
int  g_bIBeamHalo = false;

BRect g_cWinRect( 100, 200, 800, 500 );

extern char** environ;

int g_nMasterPTY;

thread_id g_hUpdateThread;

static volatile bool g_bRun = true;

class MyWindow : public BWindow
{
public:
	MyWindow( BRect cFrame,
			const char* pzTitle,
			window_type inType,
		int nFlags );
	virtual ~MyWindow();

	virtual bool        QuitRequested();
	virtual void        MessageReceived( BMessage* pcMessage );
};


class MyApp : public BApplication
{
public:
	MyApp();
	virtual ~MyApp();

	virtual bool        QuitRequested();
};

MyWindow*  g_pcWindow     = NULL;
TermView*  g_pcTermView   = NULL;
BScrollBar* g_pcScrollBar = NULL;


MyApp::MyApp() : BApplication( "application/x-vnd.KHS-aterm" )
{
}

MyApp::~MyApp()
{
}

bool MyApp::QuitRequested()
{
	return( true );
}

MyWindow::MyWindow( BRect cFrame, const char* pzTitle, window_type inType, int nFlags )
    : BWindow( cFrame, pzTitle, inType, nFlags )
{
}

MyWindow::~MyWindow()
{
	g_pcWindow = NULL;
	g_bRun = false;
	close( g_nMasterPTY );
}

void MyWindow::MessageReceived( BMessage* pcMsg )
{
	switch( pcMsg->what )
	{
		case ID_SCROLL:
			g_pcTermView->ScrollBack( g_pcScrollBar->Value() );
			break;
/*
		case 12345678:
		{
			BMessageQueue*        pcQueue = MessageQueue();

			if ( NULL == pcQueue->FindMessage( 12345678, 0 ) ) { // Wait for the last event
				g_pcTermView->RefreshDisplay( true );
			}
			break;
		}
		*/
	default:
		BWindow::MessageReceived( pcMsg );
	}
}


bool MyWindow::QuitRequested()
{
	return( true );
}


bool OpenWindow()
{
	g_pcWindow = new MyWindow( g_cWinRect, "Cosmoe Terminal", B_TITLED_WINDOW, 0);

	BRect cTermFrame   = g_pcWindow->Bounds();
	BRect cScrollBarFrame = cTermFrame;

	//g_pcScrollBar = new BScrollBar( cScrollBarFrame, "", new BMessage( ID_SCROLL ), 0, 24 );
	g_pcScrollBar = new BScrollBar( cScrollBarFrame, "", NULL, 0, 24.0, B_VERTICAL );

	float width;
	g_pcScrollBar->GetPreferredSize(&width, NULL);
	cScrollBarFrame.left = cScrollBarFrame.right - width;
	g_pcScrollBar->SetFrame( cScrollBarFrame );

	cTermFrame.right = cScrollBarFrame.left - 1;

	g_pcTermView  = new TermView( cTermFrame, "", B_FOLLOW_ALL, B_WILL_DRAW );

	g_pcWindow->AddChild( g_pcTermView );
	g_pcWindow->AddChild( g_pcScrollBar );
	g_pcScrollBar->SetTarget(g_pcTermView);

	IPoint cGlypSize( g_pcTermView->GetGlyphSize() );
	IPoint cSizeOffset( int(cScrollBarFrame.Width()) % int(cGlypSize.x), 0 );
	g_pcWindow->SetWindowAlignment( B_PIXEL_ALIGNMENT,
									1,
									0,
									cGlypSize.x,
									cSizeOffset.x,
									1,
									0,
									cGlypSize.y,
									cSizeOffset.y);
	g_pcWindow->ResizeTo( cGlypSize.x * 80 + cScrollBarFrame.Width(), cGlypSize.y * 24 );
	/*
	if ( g_pcWindow->Frame().right >= BScreen().GetResolution().x )
	{
		g_pcWindow->MoveTo( BScreen().GetResolution().x / 2 - g_pcWindow->Frame().Width() * 0.5f, g_cWinRect.top );
	}

	if ( g_pcWindow->Frame().bottom >= BScreen().GetResolution().y )
	{
		g_pcWindow->MoveTo( g_pcWindow->Frame().left, BScreen().GetResolution().y / 2 - g_pcWindow->Frame().Height() * 0.5f );
	}
	*/
	g_pcWindow->Activate( true );
	g_pcWindow->Show();

	return true;
}

static char* g_ttydev = NULL;
/*
 * see rxvt source, command.c
 * linux config defines PTYS_ARE_GETPT and PTYS_ARE_SEARCHED
 */
int GetPTY()
{
	int fd;

#ifdef __APPLE__
	char tty_name[sizeof "/dev/pts/????\0"];

	if (openpty(&fd, NULL, tty_name, NULL, NULL) != -1) {
		g_ttydev = strdup(tty_name);
	}
#else
	char*           ptydev;
	char            pty_name[] = "/dev/pty??";
	const char      PTYCHAR1[] = "pqrstuvwxyz";
	const char      PTYCHAR2[] = "0123456789abcdef";
	int             len;
	const char     *c1, *c2;
	char            tty_name[] = "/dev/tty??";

	// PTYS_ARE_GETPT
	if( (fd = getpt()) >= 0 )
	{
		if( grantpt( fd ) == 0 && unlockpt( fd ) == 0 )
		{
			g_ttydev = ptsname( fd );
			goto found;
		}
	}

	// PTYS_ARE_SEARCHED
	len = sizeof(pty_name) - 3;
	ptydev = pty_name;
	g_ttydev = tty_name;

	for( c1 = PTYCHAR1; *c1; c1++ )
	{
		ptydev[len] = g_ttydev[len] = *c1;
		for( c2 = PTYCHAR2; *c2; c2++ )
		{
			ptydev[len + 1] = g_ttydev[len + 1] = *c2;
			if( (fd = open(ptydev, O_RDWR)) >= 0 )
			{
				if( access(g_ttydev, R_OK | W_OK) == 0 )
				{
					g_ttydev = strdup( tty_name );
					goto found;
				}
				close(fd);
			}
		}
	}

	return -1;
found:
#endif

	fcntl( fd, F_SETFL, O_NDELAY );
	return fd;
}

int32 ReadPTY(void*)
{
	char zBuf[ 4096 ];

	int        nBytesRead;

	signal( SIGHUP, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGALRM, SIG_IGN );
	signal( SIGCHLD, SIG_IGN );
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	while( g_bRun && NULL != g_pcWindow )
	{
		nBytesRead = read( g_nMasterPTY, zBuf, 4096 );

		if ( nBytesRead < 0 ) {
			snooze( 40000 );
//            dbprintf( "Failed to read from tty %d\n", nBytesRead );
		}
/*
	if ( nBytesRead == 256 ) {
	SetTaskPri( g_hUpdateThread, 0 );
	} else {
	SetTaskPri( g_hUpdateThread, 0 );
	}
	*/
		if ( nBytesRead > 0 )
		{
			if ( g_pcWindow->Lock() )
			{
				g_pcTermView->Write( zBuf, nBytesRead );
				g_pcWindow->Unlock();
			}
		}
	}

	return 0;
}

int32 RefreshThread( void* pData )
{
	signal( SIGHUP, SIG_IGN );
	signal( SIGINT, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGALRM, SIG_IGN );
	signal( SIGCHLD, SIG_IGN );
	signal( SIGTTIN, SIG_IGN );
	signal( SIGTTOU, SIG_IGN );

	while ( g_bRun && NULL != g_pcWindow )
	{
		if ( g_pcWindow->Lock() )	// NLS
		{
			g_pcTermView->RefreshDisplay( false );
			g_pcWindow->Unlock();
		}
		snooze( 40000 );
	}

	return 0;
}

#ifndef __APPLE__
static struct option const long_opts[] =
{
	{"debug", required_argument, NULL, 'd' },
	{"frame", required_argument, NULL, 'f' },
	{"def_attr", required_argument, NULL, 'a' },
	{"ibeam_halo", no_argument, &g_bIBeamHalo, 1 },
	{"noborder", no_argument, NULL, 'b' },
	{"help", no_argument, &g_nShowHelp, 1},
	{"version", no_argument, &g_nShowVersion, 1},
	{NULL, 0, NULL, 0}
};
#endif

static void usage( const char* pzName, bool bFull )
{
    printf( "Usage: %s [-hfbdv] [command [args]]\n", pzName );
    if ( bFull )
    {
        printf( "  -d --debug=level   set amount of debug output to send to parent terminal\n" );
        printf( "  -f --frame=l,t,r,b set window position/size (left,top,right,bottom)\n" );
        printf( "  -a --def_attr=attr set default display attribs (fg/bg color & bold/underline)\n" );
        printf( "  -i --ibeam_halo    put an white halo around the i-beam cursor\n" );
        printf( "  -b --noborder      hide the window border\n" );
        printf( "  -h --help          display this help and exit\n" );
        printf( "  -v --version       display version information and exit\n" );
    }
}
int main( int argc, char** argv )
{
	thread_id hShellThread;
	thread_id g_hReadThread;
	char            zShellPath[PATH_MAX] = "/bin/bash";
	int            i;
	char*            apzDefaultShellArgv[2] = { zShellPath, NULL };
	char**            apzShellArgv = apzDefaultShellArgv;
	const char* pzDefAttr;
#ifndef __APPLE__
	int            c;
#endif

	pzDefAttr = getenv( "ATERM_ATTR" );

	if ( pzDefAttr != NULL ) {
		g_nDefaultAttribs = strtol( pzDefAttr, NULL, 0 );
	}

	struct passwd* psPW = getpwuid( getuid() );

	if ( psPW != NULL ) {
		strcpy( zShellPath, psPW->pw_shell );
	}

#ifndef __APPLE__
    while( (c = getopt_long (argc, argv, "hvid:f:ba:", long_opts, (int *) 0)) != EOF )
    {
        switch( c )
        {
            case 0:
                break;
            case 'h':
                g_nShowHelp = true;
                break;
            case 'v':
                g_nShowVersion = true;
                break;
            case 'i':
                g_bIBeamHalo = true;
                break;
            case 'd':
                printf( "Debug level = %s\n", optarg );
                g_nDebugLevel = atol( optarg );
                break;
            case 'f':
                sscanf( optarg, "%f,%f,%f,%f",
                        &g_cWinRect.left,&g_cWinRect.top,&g_cWinRect.right,&g_cWinRect.bottom );
                break;
            case 'b':
                g_bWindowBorder = false;
                break;
            case 'a':
                g_nDefaultAttribs = strtol( optarg, NULL, 0 );
                break;
            default:
                usage( argv[0], false );
                exit( 1 );
                break;
        }
    }
#endif

	if ( g_nShowVersion )
	{
		printf( "Cosmoe virtual terminal V0.1.2\n" );
		exit( 0 );
	}

	if ( g_nShowHelp )
	{
		usage( argv[0], true );
		exit( 0 );
	}

	if ( optind < argc )
	{
		// are the argv array NULL terminated?
		apzShellArgv = (char**)malloc( (1+argc-optind)*sizeof(char*) );
		memcpy( apzShellArgv, argv+optind, (argc-optind)*sizeof(char*) );
		apzShellArgv[argc-optind] = NULL;
	}
/*
	if ( argc > 1 ) {
	pzCommand = argv[1];
	}
	*/
/*        setpgid( 0, 0 ); */

	g_nMasterPTY = GetPTY();
	if( g_nMasterPTY < 0 )
	{
		fprintf( stderr, "cannot get a pty\n" );
		exit( 1 );
	}

	hShellThread = fork();

	if ( 0 == hShellThread )
	{
		int nSlavePTY;

		if ( setsid() == -1 )
		{
			printf( "setsid() failed - %s\n", strerror( errno ) );
		}
		nSlavePTY = open( g_ttydev, O_RDWR );
		tcsetpgrp( nSlavePTY, getpgrp() );

		termios        sTerm;

		tcgetattr( nSlavePTY, &sTerm );
		sTerm.c_oflag |= ONLCR;
		tcsetattr( nSlavePTY, TCSANOW, &sTerm );

		dup2( nSlavePTY, 0 );
		dup2( nSlavePTY, 1 );
		dup2( nSlavePTY, 2 );

		struct winsize sWinSize;

		sWinSize.ws_col          = 80;
		sWinSize.ws_row          = 24;
		sWinSize.ws_xpixel = 80 * 8;
		sWinSize.ws_ypixel = 24 * 8;

		ioctl( nSlavePTY, TIOCSWINSZ, &sWinSize );

		for ( i = 3 ; i < 256 ; ++i )
		{
			close( i );
		}

		if ( g_nDebugLevel > 0 )
		{
			printf( "Execute command %s\n", apzShellArgv[0] );
		}
//    setenv( "TERM", "xterm-color", true );
		setenv( "TERM", "xterm", true );
		execve( apzShellArgv[0], apzShellArgv, environ );
		printf( "Failed to execute %s\n", apzShellArgv[0] );
		sleep( 2 ); // give the user a chance to the the error
		exit( 1 );
	}
	else
	{
		signal( SIGHUP, SIG_IGN );
		signal( SIGINT, SIG_IGN );
		signal( SIGQUIT, SIG_IGN );
		signal( SIGALRM, SIG_IGN );
		signal( SIGCHLD, SIG_DFL );
		signal( SIGTTIN, SIG_IGN );
		signal( SIGTTOU, SIG_IGN );

		MyApp cMyApp;
		cMyApp.Unlock();

		OpenWindow();

		g_hUpdateThread = spawn_thread( RefreshThread, "update", 5, NULL );
		resume_thread( g_hUpdateThread );

		g_hReadThread = spawn_thread( ReadPTY, "read", 10, NULL );
		resume_thread( g_hReadThread );

		while( g_bRun && NULL != g_pcWindow )
		{
			pid_t hPid = waitpid( hShellThread, NULL, 0 );
			if (  hPid == hShellThread )
			{
				g_bRun = false;
				waitpid( g_hUpdateThread, NULL, 0 );
				waitpid( g_hReadThread, NULL, 0 );
				if ( NULL != g_pcWindow && g_pcWindow->Lock() )
				{
					g_pcWindow->PostMessage( B_QUIT_REQUESTED );
					g_pcWindow->Unlock();
				}
			}

			if ( hPid < 0  )
			{
				break;
			}
		}

/*        kill( g_hReadThread, SIGKILL ); */
/*
kill( hShellThread, SIGKILL );
kill( g_hReadThread, SIGKILL );
*/
	}
	return( 0 );
}
