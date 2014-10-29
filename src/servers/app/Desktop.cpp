//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		Desktop.cpp
//	Author:			Adi Oanca <adioanca@mymail.ro>
//	Description:	Class used to encapsulate desktop management
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <dirent.h>
#include <assert.h>

#include <OS.h>
#include <image.h>

#include <Screen.h>
#include <Window.h>

#include <Message.h>
#include <Messenger.h>

#include "ServerWindow.h"
#include "AppServer.h"
#include "config.h"
#include "Desktop.h"
#include "CursorHandler.h"
#include "DisplayDriver.h"
#include "Layer.h"
#include "ServerConfig.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include <PortLink.h>

//#define REAL_MODE
#include "ServerBitmap.h"
#include "sprite.h"
#include "../config.h"

#ifdef COSMOE_DIRECTFB
#include "dfbdriver.h"
#define DRIVER_CLASS DirectFBDriver
#define DRIVER_NAME "DirectFB Driver"
#endif


#ifdef COSMOE_SDL
#include "sdldriver.h"
#define DRIVER_CLASS SDLDriver
#define DRIVER_NAME "SDL Driver"
#endif

#ifdef COSMOE_XWINDOWS
#include "x11driver.h"
#define DRIVER_CLASS X11Driver
#define DRIVER_NAME "X11 Driver"
#endif

#include <macros.h>

#include <set>
#include <algorithm>

#define DEBUG_DESKTOP
#define DEBUG_KEYHANDLING

#ifdef DEBUG_DESKTOP
	#define STRACE(a) printf a 
#else
	#define STRACE(a) /* nothing */
#endif

static int32 g_nActiveDesktop = -1;

#define FOCUS_STACK_SIZE  32
#define NUM_WORKSPACES 32

static struct Desktops
{
	ServerWindow*  m_pcFirstWindow;
	ServerWindow*  m_pcFirstSystemWindow;
	ServerWindow*  m_pcActiveWindow;
	ServerWindow*  m_apcFocusStack[FOCUS_STACK_SIZE];
	display_mode displayMode;
	std::string m_cBackdropPath;
} g_asDesktops[NUM_WORKSPACES];


Desktop::Desktop(void)
{
	fDragMessage		= NULL;
	fActiveScreen		= NULL;
	fScreenShotIndex	= 1;
}

Desktop::~Desktop(void)
{
	if (fDragMessage)
		delete fDragMessage;

	void	*ptr;

	for(int32 i=0; (ptr=fScreenList.ItemAt(i)); i++)
		delete (Screen*)ptr;
}

void Desktop::Init(void)
{
STRACE(("Desktop: InitDesktop\n"));
	DisplayDriver	*driver = NULL;
	int32 driverCount = 0;
	bool initDrivers = true;

	while(initDrivers)
	{
		driver = new DRIVER_CLASS;
		STRACE(( "Loading " DRIVER_NAME "...\n" ));

		if(driver->Initialize())
		{
			STRACE(( DRIVER_NAME " succesfully initialized\n" ));
			driverCount++;

			Screen		*sc = new Screen(driver, BPoint(800, 600), B_RGB32, driverCount);

			// TODO: be careful, of screen initialization - monitor may not support 640x480
			fScreenList.AddItem(sc);

			// TODO: remove this when you have a real Driver.
			if (driverCount == 1)
				initDrivers	= false;
		}
		else
		{
			STRACE(( DRIVER_NAME "FAILED initialization - game over\n" ));
			driver->Shutdown();
			delete	driver;
			driver	= NULL;
			initDrivers	= false;
		}
	}

	if (driverCount < 1){
		delete this;
		return;
	}

	InitMode();

	g_nActiveDesktop = 0;
	driver->GetMode(&(g_asDesktops[g_nActiveDesktop].displayMode));
	
	// Create root layer
#if defined(COSMOE_DIRECTFB)
	// A total hack, so hackish that it doesn't even work
	g_pcTopView = new Layer(new UtilityBitmap(BRect(0, 0, 799, 599), B_RGB32, 0), true );
#else
	g_pcTopView = new Layer( ((BitmapDriver*)driver)->GetTarget(), true );
#endif
}

void Desktop::InitMode(void)
{
	// this is init mode for n-SS.
	fActiveScreen	= fScreenList.ItemAt(0)? (Screen*)fScreenList.ItemAt(0): NULL;
}

//---------------------------------------------------------------------------
//					Methods for multiple monitors.
//---------------------------------------------------------------------------


Screen* Desktop::ScreenAt(int32 index) const
{
	Screen	*sc= static_cast<Screen*>(fScreenList.ItemAt(index));

	return sc;
}

int32 Desktop::ScreenCount(void) const
{
	return fScreenList.CountItems();
}

Screen* Desktop::ActiveScreen(void) const
{
	return fActiveScreen;
}

DisplayDriver* Desktop::GetDisplayDriver() const
{
	return ScreenAt(0)->DDriver();
}

//---------------------------------------------------------------------------
//				Methods for layer(WinBorder) manipulation.
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//				Input related methods
//---------------------------------------------------------------------------
void Desktop::MouseEventHandler(int32 code, BPortLink& msg)
{
	// TODO: locking mechanism needs SERIOUS rethought
	switch(code)
	{
		case B_MOUSE_DOWN:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down
			// 5) int32 - buttons down
			// 6) int32 - clicks

			PointerEvent evt;	
			evt.code = B_MOUSE_DOWN;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.modifiers);
			msg.Read<int32>(&evt.buttons);
			msg.Read<int32>(&evt.clicks);
			
			BMessage downmsg(B_MOUSE_DOWN);
			
			downmsg.AddInt64("when", evt.when);
			downmsg.AddInt32("modifiers", evt.modifiers);
			downmsg.AddPoint("where", evt.where);
			downmsg.AddInt32("buttons", evt.buttons);
		
			ServerWindow::HandleInputEvent(&downmsg);
			
			printf("MOUSE DOWN: at (%f, %f)\n", evt.where.x, evt.where.y);
			break;
		}
		case B_MOUSE_UP:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down

			PointerEvent evt;	
			evt.code = B_MOUSE_UP;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.modifiers);
			msg.Read<int32>(&evt.buttons);

			BMessage upmsg(B_MOUSE_UP);
			upmsg.AddInt64("when",evt.when);
			upmsg.AddPoint("where",evt.where);
			upmsg.AddInt32("modifiers",evt.modifiers);
			upmsg.AddInt32("buttons",evt.buttons);

			ServerWindow::HandleInputEvent(&upmsg);
			
			STRACE(("MOUSE UP: at (%f, %f)\n", evt.where.x, evt.where.y));

			break;
		}
		case B_MOUSE_MOVED:
		{
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - buttons down
			
			PointerEvent evt;	
			evt.code = B_MOUSE_MOVED;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.buttons);

			fActiveScreen->DDriver()->MoveCursorTo(evt.where.x, evt.where.y);

			BMessage movemsg(B_MOUSE_MOVED);
			movemsg.AddInt64("when",evt.when);
			movemsg.AddPoint("where",evt.where);
			movemsg.AddInt32("buttons",evt.buttons);
			
			ServerWindow::HandleInputEvent(&movemsg);
			break;
		}
		case B_MOUSE_WHEEL_CHANGED:
		{
			// FEATURE: This is a tentative change: mouse wheel messages are always sent to the window
			// under the cursor. It's pretty stupid to send it to the active window unless a particular
			// view has locked focus via SetMouseEventMask
			break;
		}
		default:
		{
			printf("\nDesktop::MouseEventHandler(): WARNING: unknown message\n\n");
			break;
		}
	}
}

void Desktop::KeyboardEventHandler(int32 code, BPortLink& msg)
{

	switch(code)
	{
		case B_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 repeat count
			// 5) int32 modifiers
			// 6) int8[3] UTF-8 data generated
			// 7) int8 number of bytes to follow containing the 
			//		generated string
			// 8) Character string generated by the keystroke
			// 9) int8[16] state of all keys
			
			bigtime_t time;
			int32 scancode, modifiers;
			char *string = NULL;
			char *convString = NULL;

			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&modifiers);
			msg.ReadString(&string);
			msg.ReadString(&convString);

			g_cLayerGate.Lock();
			ServerWindow* pcWnd = GetActiveWindow(false);
			if ( pcWnd != NULL )
			{
				BMessage cMsg(B_KEY_DOWN);

				cMsg.AddPointer( "_widget", pcWnd->GetClientView()->GetUserObject() );
				cMsg.AddInt32( "_raw_key", scancode );
				cMsg.AddInt32( "modifiers", modifiers );
				cMsg.AddString( "_string", convString );
				cMsg.AddString( "_raw_string", string );

				pcWnd->SendMessageToClient( &cMsg );
			}
			g_cLayerGate.Unlock();
			break;
		}
		case B_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 modifiers
			// 5) int8[3] UTF-8 data generated
			// 6) int8 number of bytes to follow containing the 
			//		generated string
			// 7) Character string generated by the keystroke
			// 8) int8[16] state of all keys
			
			// Obtain only what data which we'll need

			// We got this far, so apparently it's safe to pass to the active
			// window.
			
			// TODO: Pass on key up message to client window with the focus
			break;
		}
		case B_UNMAPPED_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			bigtime_t time;
			int32 scancode;
			int32 modifiers;
			int32 elements;

			#ifdef DEBUG_KEYHANDLING
			printf("Unmapped Key Down: 0x%lx\n", scancode);
			#endif
			
			// TODO: Pass on unmapped key down message to client window with the focus
			break;
		}
		case B_UNMAPPED_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys

			bigtime_t time;
			int32 scancode;
			int32 modifiers;
			int32 elements;

			#ifdef DEBUG_KEYHANDLING
			printf("Unmapped Key Up: 0x%lx\n", scancode);
			#endif

			// TODO: Pass on unmapped key up message to client window with the focus
			break;
		}
		case B_MODIFIERS_CHANGED:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 modifiers
			// 3) int32 old modifiers
			// 4) int32 number of elements in the key state array to follow
			// 5) int8 state of all keys
			#ifdef DEBUG_KEYHANDLING
			printf("Modifiers Changed\n");
			#endif

			// TODO: Pass on modifier change message to client window with the focus
			break;
		}
		default:
			break;
	}
}

void Desktop::SetDragMessage(BMessage* msg)
{
	if (fDragMessage)
	{
		delete fDragMessage;
		fDragMessage = NULL;
	}

	if (msg)
		fDragMessage	= new BMessage(*msg);
}

BMessage* Desktop::DragMessage(void) const
{
	return fDragMessage;
}

//---------------------------------------------------------------------------
//				Methods for various desktop stuff handled by the server
//---------------------------------------------------------------------------
void Desktop::SetScrollBarInfo(const scroll_bar_info &info)
{
	fScrollBarInfo	= info;
}

scroll_bar_info Desktop::ScrollBarInfo(void) const
{
	return fScrollBarInfo;
}

void Desktop::SetMenuInfo(const menu_info &info)
{
	fMenuInfo	= info;
}

menu_info Desktop::MenuInfo(void) const
{
	return fMenuInfo;
}

void Desktop::UseFFMouse(const bool &useffm)
{
	fFFMouseMode	= useffm;
}

bool Desktop::FFMouseInUse(void) const
{
	return fFFMouseMode;
}

void Desktop::SetFFMouseMode(const mode_mouse &value)
{
	fMouseMode	= value;
}

mode_mouse Desktop::FFMouseMode(void) const
{
	return fMouseMode;
}

void Desktop::PrintToStream(void)
{
	
	printf("Screen List:\n");
	for(int32 i=0; i<fScreenList.CountItems(); i++)
		printf("\t%ld\n", ((Screen*)fScreenList.ItemAt(i))->ScreenNumber());
}

int  get_desktop_config( int32* pnActiveDesktop, display_mode* psMode, std::string* pcBackdropPath )
{
	if ( *pnActiveDesktop == B_MAIN_SCREEN_ID.id )
	{
		*pnActiveDesktop = g_nActiveDesktop;
	}

	if ( psMode != NULL )
	{
		*psMode = g_asDesktops[*pnActiveDesktop].displayMode;
	}

	if ( pcBackdropPath != NULL )
	{
		*pcBackdropPath = g_asDesktops[*pnActiveDesktop].m_cBackdropPath;
	}
	return( 0 );
}


/*!
	\brief Returns the active window in the current workspace of the active screen
	\return The active window in the current workspace of the active screen
*/
ServerWindow *GetActiveWindow(bool bIgnoreSystemWindows)
{
	if ( bIgnoreSystemWindows == false && g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow != NULL )
	{
		return( g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow );
	}

	return( g_asDesktops[g_nActiveDesktop].m_pcActiveWindow );
}

/*!
	\brief Sets the active window in the current workspace of the active screen
	\param win The window to activate
	
	If the window is not in the current workspace of the active screen, this call fails
*/
void SetActiveWindow(ServerWindow *win, bool bNotifyPrevious)
{
STRACE(("Desktop: SetActiveWindow(%s)\n",win?win->Title():"NULL"));

	if(win && (win->Flags() & WND_SYSTEM) )
	{
		return;
	}

	if ( win == NULL && g_asDesktops[g_nActiveDesktop].m_pcFirstSystemWindow != NULL )
	{
		return;
	}

	if ( win == g_asDesktops[g_nActiveDesktop].m_pcActiveWindow )
	{
		return;
	}

	ServerWindow* pcPrevious = g_asDesktops[g_nActiveDesktop].m_pcActiveWindow;

	if ( win == NULL )
	{
		g_asDesktops[g_nActiveDesktop].m_pcActiveWindow = g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0];
		memmove( &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0],
				 &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[1],
				 sizeof( ServerWindow* ) * (FOCUS_STACK_SIZE - 1) );
		g_asDesktops[g_nActiveDesktop].m_apcFocusStack[FOCUS_STACK_SIZE-1] = NULL;
	}
	else
	{
		memmove( &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[1],
				 &g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0],
				 sizeof( ServerWindow* ) * (FOCUS_STACK_SIZE - 1) );
		g_asDesktops[g_nActiveDesktop].m_apcFocusStack[0] = g_asDesktops[g_nActiveDesktop].m_pcActiveWindow;
		g_asDesktops[g_nActiveDesktop].m_pcActiveWindow = win;
	}

	if ( pcPrevious != g_asDesktops[g_nActiveDesktop].m_pcActiveWindow )
	{
		if ( bNotifyPrevious && pcPrevious != NULL )
		{
			pcPrevious->WindowActivated( false );
		}
		if ( g_asDesktops[g_nActiveDesktop].m_pcActiveWindow != NULL )
		{
			g_asDesktops[g_nActiveDesktop].m_pcActiveWindow->WindowActivated( true );
		}
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void AddWindowToDesktop( ServerWindow* pcWindow )
{
	uint32 nMask = pcWindow->GetWorkspaceIndex();

	for ( int i = 0 ; i < 32 ; ++i )
	{
		if ( nMask & (1 << i) )
		{
			pcWindow->m_asDTState[i].m_pcNextWindow = g_asDesktops[i].m_pcFirstWindow;
			g_asDesktops[i].m_pcFirstWindow = pcWindow;

			if ( pcWindow->Flags() & WND_SYSTEM )
			{
				pcWindow->m_asDTState[i].m_pcNextSystemWindow = g_asDesktops[i].m_pcFirstSystemWindow;
				g_asDesktops[i].m_pcFirstSystemWindow = pcWindow;
			}

			if ( i == g_nActiveDesktop )
			{
				g_pcTopView->AddChild( pcWindow->GetTopView(), true );
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void RemoveWindowFromDesktop( ServerWindow* pcWindow )
{
	uint32 nMask = pcWindow->GetWorkspaceIndex();

	for ( int i = 0 ; i < 32 ; ++i )
	{
		if ( nMask & (1 << i) )
		{
			ServerWindow** ppcTmp;
			bool bFound = false;

			for ( ppcTmp = &g_asDesktops[i].m_pcFirstWindow ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_asDTState[i].m_pcNextWindow )
			{
				if ( *ppcTmp == pcWindow )
				{
					*ppcTmp = pcWindow->m_asDTState[i].m_pcNextWindow;
					pcWindow->m_asDTState[i].m_pcNextWindow = NULL;
					bFound = true;
					break;
				}
			}

			if ( pcWindow->Flags() & WND_SYSTEM )
			{
				for ( ppcTmp = &g_asDesktops[i].m_pcFirstSystemWindow ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_asDTState[i].m_pcNextSystemWindow )
				{
					if ( *ppcTmp == pcWindow )
					{
						*ppcTmp = pcWindow->m_asDTState[i].m_pcNextSystemWindow;
						pcWindow->m_asDTState[i].m_pcNextSystemWindow = NULL;
						break;
					}
				}
			}

			for ( int j = 0 ; j < FOCUS_STACK_SIZE ; ++j )
			{
				if ( g_asDesktops[i].m_apcFocusStack[j] == pcWindow )
				{
					for ( int k = j ; k < FOCUS_STACK_SIZE - 1 ; ++k )
					{
						g_asDesktops[i].m_apcFocusStack[k] = g_asDesktops[i].m_apcFocusStack[k+1];
					}
					g_asDesktops[i].m_apcFocusStack[FOCUS_STACK_SIZE-1] = NULL;
				}
			}
			__assertw( bFound );
			if ( i == g_nActiveDesktop )
			{
				pcWindow->GetTopView()->RemoveSelf();
			}
			else
			{
				if ( pcWindow == g_asDesktops[i].m_pcActiveWindow )
				{
					g_asDesktops[i].m_pcActiveWindow = NULL;
				}
			}
		}
	}
	if ( pcWindow == GetActiveWindow(true) )
	{
		SetActiveWindow( NULL, false );
	}
}


int get_active_desktop()
{
	return( g_nActiveDesktop );
}


