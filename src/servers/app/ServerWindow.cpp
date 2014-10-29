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
//	File Name:		ServerWindow.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Shadow BWindow class
//  
//------------------------------------------------------------------------------

#include <AppDefs.h>
#include <Rect.h>
#include <string.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>

#include <SupportDefs.h>

#include "ServerFont.h"

#include "AppServer.h"
#include "Layer.h"
#include "ServerWindow.h"
#include "ServerApp.h"
#include <ServerProtocol.h>
#include "WinBorder.h"
#include "Decorator.h"
#include "Desktop.h"
#include "DisplayDriver.h"

#include <ServerBitmap.h>
#include "sprite.h"
#include "config.h"

#include <kernel.h>

#include <InterfaceDefs.h>
#include <Messenger.h>

#include <macros.h>

//#define DEBUG_SERVERWINDOW
//#define DEBUG_SERVERWINDOW_MOUSE
//#define DEBUG_SERVERWINDOW_KEYBOARD

#ifdef DEBUG_SERVERWINDOW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_SERVERWINDOW_KEYBOARD
#	include <stdio.h>
#	define STRACE_KEY(x) printf x
#else
#	define STRACE_KEY(x) ;
#endif

#ifdef DEBUG_SERVERWINDOW_MOUSE
#	include <stdio.h>
#	define STRACE_MOUSE(x) printf x
#else
#	define STRACE_MOUSE(x) ;
#endif

ServerWindow* ServerWindow::s_pcLastMouseWindow = NULL;
ServerWindow* ServerWindow::s_pcDragWindow      = NULL;
SrvSprite* ServerWindow::s_pcDragSprite      = NULL;
BMessage   ServerWindow::s_cDragMessage((uint32)0);
bool       ServerWindow::s_bIsDragging       = false;

//------------------------------------------------------------------------------

template<class Type> Type
read_from_buffer(int8 **_buffer)
{
	Type *typedBuffer = (Type *)(*_buffer);
	Type value = *typedBuffer;

	typedBuffer++;
	*_buffer = (int8 *)(typedBuffer);

	return value;
}
//------------------------------------------------------------------------------
/*
static int8 *read_pattern_from_buffer(int8 **_buffer)
{
	int8 *pattern = *_buffer;

	*_buffer += AS_PATTERN_SIZE;

	return pattern;
}
*/
//------------------------------------------------------------------------------
template<class Type> void
write_to_buffer(int8 **_buffer, Type value)
{
	Type *typedBuffer = (Type *)(*_buffer);

	*typedBuffer = value;
	typedBuffer++;

	*_buffer = (int8 *)(typedBuffer);
}
//------------------------------------------------------------------------------
/*!
	\brief Contructor
	
	Does a lot of stuff to set up for the window - new decorator, new winborder, spawn a 
	monitor thread.
*/
ServerWindow::ServerWindow(BRect rect, const char *string, uint32 wlook, uint32 wfeel, 
void* pTopView,
	uint32 wflags, ServerApp *winapp, port_id winport, uint32 index) :
    fLocker( "swindow_lock" )
{
	STRACE(("ServerWindow(%s)::ServerWindow()\n",string? string: "NULL"));
	fServerApp			= winapp;

	if(string)
		fTitle.SetTo(string);
	else
		fTitle.SetTo(B_EMPTY_STRING);
		
	fFrame = rect;
	fFlags = wflags;
	fLook = wlook;
	fFeel = wfeel;
	
	// fClientWinPort is the port to which the app awaits messages from the server
	fClientWinPort = winport;

	// fMessagePort is the port to which the app sends messages for the server
	_receiver = create_port(30,fTitle.String());

	fIsActive			= false;

	_appTarget = new BMessenger( -1, winport, -1, false );

	m_bOffscreen  = false;
	m_bNeedRegionUpdate = false;
	BRect cBorderFrame = rect;

	fWinBorder = new WinBorder( this, NULL, "wnd_border", (wflags & WND_BACKMOST) );
	m_pcDecorator = new_decorator( fWinBorder, fLook, fFeel, wflags );
	fWinBorder->SetDecorator( m_pcDecorator );

	fTopLayer = fWinBorder->GetClient();
	printf("fTopLayer is %p\n", fTopLayer);
	fflush(stdout);
	fTopLayer->SetUserObject( pTopView );

	BRect cBorders = m_pcDecorator->GetBorderSize();

	cBorderFrame.Set( cBorderFrame.left - cBorders.left,
					  cBorderFrame.top - cBorders.top,
					  cBorderFrame.right + cBorders.right,
					  cBorderFrame.bottom + cBorders.bottom );

	fWinBorder->SetFrame( cBorderFrame );
	m_pcDecorator->SetTitle( string );

	m_bBorderHit = false;

	if ( index == 0 )
	{
		fWorkspaces = 1 << get_active_desktop();
	}
	else
	{
		fWorkspaces = index;
	}

	for ( int i = 0 ; i < 32 ; ++i )
	{
		m_asDTState[i].m_cFrame = rect;
	}

	STRACE(("ServerWindow %s:\n",fTitle.String()));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom));
	STRACE(("\tPort: %ld\n",_receiver));
	STRACE(("\tWorkspace: %ld\n",index));
}
//------------------------------------------------------------------------------
void ServerWindow::Init(void)
{
	// NOTE: this MUST be before the monitor thread is spawned!
	AddWindowToDesktop(this);

	// Spawn our message-monitoring thread
	fMonitorThreadID = spawn_thread(MonitorWin, fTitle.String(), B_NORMAL_PRIORITY, this);
	if(fMonitorThreadID != B_NO_MORE_THREADS && fMonitorThreadID != B_NO_MEMORY)
		resume_thread(fMonitorThreadID);


}
//------------------------------------------------------------------------------
//!Tears down all connections the main app_server objects, and deletes some internals.
ServerWindow::~ServerWindow(void)
{
	STRACE(("*ServerWindow (%s):~ServerWindow()\n",fTitle.String()));

	if ( this == s_pcLastMouseWindow )
		s_pcLastMouseWindow = NULL;

	if ( this == s_pcDragWindow )
		s_pcDragWindow = NULL;

	if ( NULL != fWinBorder )
	{
		Layer* pcParent = fWinBorder->GetParent();
		RemoveWindowFromDesktop( this );
		if ( pcParent != NULL )
		{
			pcParent->UpdateRegions();
		}
		delete fWinBorder;
	}
	else
	{
		delete fTopLayer;
	}
	delete _appTarget;

	if ( _receiver != -1 )
	{
		delete_port( _receiver );
	}
}


//! Forces the window border to update its decorator
void ServerWindow::ReplaceDecorator(void)
{
	STRACE(("ServerWindow %s: Replace Decorator\n",fTitle.String()));
	if ( fWinBorder == NULL )
	{
		return;
	}

	BRect cFrame = Frame();

	m_pcDecorator = new_decorator( fWinBorder, fLook, fFeel, fFlags );
	fWinBorder->SetDecorator( m_pcDecorator );
	m_pcDecorator->SetTitle( fTitle.String() );
	if ( HasFocus() )
	{
		m_pcDecorator->SetFocus( true );
	}
	SetFrame( cFrame );
}


//! Requests that the ServerWindow's BWindow quit
void ServerWindow::Quit(void)
{
STRACE(("ServerWindow %s: Quit\n",fTitle.String()));
	if ( _receiver != -1 )
	{
		write_port( _receiver, B_QUIT_REQUESTED, NULL, 0 );
	}
	else
	{
		ServerApp*        pcApp = App();
		__assertw( NULL != pcApp );
		AR_DeleteWindow_s sReq;
		sReq.pcWindow        = this;
		write_port( pcApp->_receiver, AS_DELETE_WINDOW, &sReq, sizeof(sReq) );
	}
}


void ServerWindow::Show(void)
{
STRACE(("ServerWindow %s: Show\n",fTitle.String()));
	if (fWinBorder)
	{
		fWinBorder->Show();
	}
}
//------------------------------------------------------------------------------
//! Hides the window's WinBorder
void ServerWindow::Hide(void)
{
	STRACE(("ServerWindow %s: Hide\n",fTitle.String()));

	if(fWinBorder)
		fWinBorder->Hide();
}

// Sends a message to the client to perform a Zoom
/*!
	\brief Handles focus and redrawing when changing focus states
	
	The ServerWindow is set to (in)active and its decorator is redrawn based on its active status
*/
void ServerWindow::SetFocus(bool value)
{
STRACE(("ServerWindow %s: Set Focus to %s\n",fTitle.String(),value?"true":"false"));
	if ( value )
	{
		SetActiveWindow( this );
	}
	else
	{
		SetActiveWindow( NULL );
	}
}

/*!
	\brief Determines whether or not the window is active
	\return true if active, false if not
*/
bool ServerWindow::HasFocus(void)
{
	return( this == GetActiveWindow(true) );
}


/*!
	\brief Notifies window of workspace (de)activation
	\param workspace Index of the workspace changed
	\param active New active status of the workspace
*/
void ServerWindow::WorkspaceActivated(int32 workspace, const IPoint cNewRes, color_space eColorSpace )
{
STRACE(("ServerWindow %s: WorkspaceActivated(%ld,%s)\n",fTitle.String(),workspace,(active)?"active":"inactive"));
	if ( _appTarget != NULL )
	{
		BMessage cMsg( M_DESKTOP_ACTIVATED );
		cMsg.AddInt32( "_new_desktop", workspace );
		cMsg.AddIPoint( "_new_resolution", cNewRes );
		cMsg.AddInt32( "_new_color_space", eColorSpace );
		if ( _appTarget->SendMessage( &cMsg ) < 0 )
		{
			printf( "Error: ServerWindow::DesktopActivated() failed to send message to target %s\n", fTitle.String() );
		}
	}
}


/*!
	\brief Notifies window of a change in focus
	\param active New active status of the window
*/
void ServerWindow::WindowActivated(bool active)
{
	STRACE(("ServerWindow %s: WindowActivated(%s)\n",fTitle.String(),(active)?"active":"inactive"));

	if ( fServerApp != NULL )
	{
		if ( _appTarget != NULL )
		{
			BMessage cMsg( B_WINDOW_ACTIVATED );

			cMsg.AddBool( "_is_active", active );
			cMsg.AddPoint( "where", desktop->GetDisplayDriver()->GetCursorPosition() );
			if ( _appTarget->SendMessage( &cMsg ) < 0 )
			{
				printf( "Error: ServerWindow::WindowActivated() failed to send message to target %s\n", fTitle.String() );
			}
		}

		if ( fWinBorder != NULL )
		{
			m_pcDecorator->SetFocus(active);
		}
	}
}

/*!
	\brief Notifies window of a change in screen resolution
	\param frame Size of the new resolution
	\param color_space Color space of the new screen mode
*/
void ServerWindow::ScreenModeChanged(const IPoint cNewRes, color_space cspace)
{
	STRACE(("ServerWindow %s: ScreenModeChanged\n",fTitle.String()));

	if ( _appTarget != NULL )
	{
		BMessage cMsg( M_SCREENMODE_CHANGED );
		cMsg.AddIPoint( "_new_resolution", cNewRes );
		cMsg.AddInt32( "_new_color_space", cspace);
		if ( _appTarget->SendMessage( &cMsg ) < 0 )
		{
			printf( "Error: ServerWindow::ScreenModeChanged() failed to send message to target %s\n", fTitle.String() );
		}
	}
}

/*
	\brief Sets the frame size of the window
	\rect New window size
*/
void ServerWindow::SetFrame(const BRect &rect)
{
	STRACE(("ServerWindow %s: Set Frame to (%.1f,%.1f,%.1f,%.1f)\n",fTitle.String(),
			rect.left,rect.top,rect.right,rect.bottom));
	if ( NULL != fWinBorder )
	{
		BRect cBorders = m_pcDecorator->GetBorderSize();
		fWinBorder->SetFrame( BRect( rect.left - cBorders.left, rect.top - cBorders.top,
									rect.right + cBorders.right, rect.bottom + cBorders.bottom ) );
	}
}


/*!
	\brief Returns the frame of the window in screen coordinates
	\return The frame of the window in screen coordinates
*/
BRect ServerWindow::Frame(bool bClient)
{
	BRect cRect;
	if ( NULL != fWinBorder )
	{
		cRect = fWinBorder->Frame();
		if ( bClient )
		{
			BRect cBorders = m_pcDecorator->GetBorderSize();

			cRect.Set( cRect.left + cBorders.left,
						cRect.top + cBorders.top,
						cRect.right - cBorders.right,
						cRect.bottom - cBorders.bottom );
		}
	}
	else
	{ // Bitmap window
		cRect = fTopLayer->Frame();
	}
	
	return( cRect );
}

/*!
	\brief Locks the window
	\return B_OK if everything is ok, B_ERROR if something went wrong
*/
status_t ServerWindow::Lock(void)
{
	STRACE(("ServerWindow %s: Lock\n",fTitle.String()));
	
	return (fLocker.Lock());
}

//! Unlocks the window
void ServerWindow::Unlock(void)
{
	STRACE(("ServerWindow %s: Unlock\n",fTitle.String()));
	
	fLocker.Unlock();
}


/*!
	\brief Determines whether or not the window is locked
	\return True if locked, false if not.
*/
bool ServerWindow::IsLocked(void) const
{
	return fLocker.IsLocked();
}

bool ServerWindow::DispatchMessage(BMessage* msg)
{
	switch(msg->what)
	{
		case AS_LAYER_CREATE:
		{
			// Received when a view is attached to a window. This will require
			// us to attach a layer in the tree in the same manner and invalidate
			// the area in which the new layer resides assuming that it is
			// visible.
			STRACE(("ServerWindow %s: AS_LAYER_CREATE...\n", fTitle.String()));
			Layer* pcParent;
			int32 hParentView = -1;

			msg->FindInt32( "parent_view", &hParentView );

			g_cLayerGate.Close();
			pcParent = FindLayer( hParentView );

			if ( pcParent != NULL )
			{
				BRect		frame;
				uint32		flags;
				void*       pUserObject;
				int32		hidden = 0;
				const char*	name;

				msg->FindString("name", &name);
				msg->FindRect("frame", &frame);
				msg->FindInt32("flags", (int32*)&flags);
				msg->FindInt32("hide_count", &hidden);
				msg->FindPointer( "user_object", &pUserObject );

				// view's visible area is invalidated here
				Layer		*newLayer;
				newLayer	= new Layer(frame, name, pcParent, flags, pUserObject, this);

				// there is no way of setting this, other than manual. :-)
				newLayer->_hidden	+= hidden;

				int32 intcolor;
				msg->FindInt32("fg_color", &intcolor);
				newLayer->SetHighColor(_get_rgb_color(intcolor));
				msg->FindInt32("bg_color", &intcolor);
				newLayer->SetLowColor(_get_rgb_color(intcolor));
				msg->FindInt32("er_color", &intcolor);
				newLayer->SetEraseColor(_get_rgb_color(intcolor));
				msg->FindPoint("scroll_offset", &newLayer->m_cScrollOffset);
				newLayer->m_cIScrollOffset = IPoint(newLayer->m_cScrollOffset);


				BMessage cReply;
				cReply.AddInt32( "handle", newLayer->GetHandle() );
				cReply.AddInt32( "error", 0 );

				msg->SendReply( &cReply );
				pcParent->UpdateRegions();
				ServerWindow::HandleMouseTransaction();
			}
			else
			{
				printf( "Error: Attempt to create view with invalid parent %ld\n", hParentView );
				BMessage cReply;
				cReply.AddInt32( "error", -EINVAL );
				msg->SendReply( &cReply );
			}
			g_cLayerGate.Open();
			STRACE(("DONE: ServerWindow %s: Message AS_CREATE_LAYER: Parent: %s, Child: %s\n", fTitle.String(), cl->_name->String(), name));
			break;
		}
		case AS_LAYER_DELETE:
		{
			// Received when a view is detached from a window. This is definitely
			// the less taxing operation - we call PruneTree() on the removed
			// layer, detach the layer itself, delete it, and invalidate the
			// area assuming that the view was visible when removed
			STRACE(("SW %s: AS_LAYER_DELETE(self)...\n", fTitle.String()));			
			Layer		*pcView;
			int32           hView = -1;

			msg->FindInt32( "handle", &hView );
			g_cLayerGate.Close();
			SrvSprite::Hide();
			if ( fTopLayer->GetHandle() != hView )
			{
				pcView = FindLayer( hView );
				if ( pcView != NULL ) {
					if ( pcView->GetWindow() != this )
					{
						printf( "Error : Attempt to delete widget not belonging to this window!\n" );
						g_cLayerGate.Open();
						break;
					}
					Layer* pcParent = pcView->GetParent();
					pcView->RemoveSelf();

					delete pcView;

					if ( pcParent != NULL )
					{
						pcParent->UpdateRegions( true );
					}
					ServerWindow::HandleMouseTransaction();
				} else {
					printf( "Error : Attempt to delete invalid view %ld\n", hView );
				}
			} else {
				fTopLayer->SetUserObject( NULL );
			}
			SrvSprite::Unhide();
			g_cLayerGate.Open();
			break;
		}
		case AS_WINDOW_TITLE:
		{
			const char* pzTitle;

			if ( msg->FindString( "title", &pzTitle ) < 0 )
			{
				printf( "Error: AS_WINDOW_TITLE message have no 'title' field\n" );
				break;
			}
			g_cLayerGate.Close();
			fTitle.SetTo(pzTitle);
			if ( m_pcDecorator != NULL )
			{
				m_pcDecorator->SetTitle( fTitle.String() );
			}

			BString threadName("W:");
			threadName.Append(pzTitle);
			rename_thread( -1, threadName.String() );
			g_cLayerGate.Open();
			break;
		}
		case AS_SET_FLAGS:
		{
			if ( fWinBorder != NULL )
			{
				int32 nFlags = fFlags;
				msg->FindInt32( "flags", &nFlags );

				g_cLayerGate.Close();

				fFlags = nFlags;
				BRect cFrame = Frame();
				fWinBorder->SetFlags( nFlags );
				SetFrame( cFrame );
				if ( cFrame != Frame() )
				{
					SrvSprite::Hide();
					g_pcTopView->UpdateRegions( false );
					ServerWindow::HandleMouseTransaction();
					SrvSprite::Unhide();
				}
				else
				{
					fWinBorder->UpdateIfNeeded( false );
				}
				g_cLayerGate.Open();
			}
			break;
		}
		case AS_SET_FEEL:
		{
			// TODO: Implement
			STRACE(("ServerWindow %s: Message Set_Feel unimplemented\n",fTitle.String()));
			break;
		}
		case AS_SET_ALIGNMENT:
		{
			if ( fWinBorder != NULL )
			{
				IPoint cSize(1,1);
				IPoint cSizeOff(0,0);
				IPoint cPos(1,1);
				IPoint cPosOff(0,0);

				msg->FindIPoint( "size", &cSize );
				msg->FindIPoint( "size_off", &cSizeOff );
				msg->FindIPoint( "pos", &cPos );
				msg->FindIPoint( "pos_off", &cPosOff );
				if ( cSize.x < 1 ) cSize.x = 1;
				if ( cSize.y < 1 ) cSize.y = 1;
				if ( cPos.x < 1 )  cPos.x  = 1;
				if ( cPos.y < 1 )  cPos.y  = 1;

				fWinBorder->SetAlignment( cSize, cSizeOff, cPos, cPosOff );
			}
			break;
		}
		case AS_SET_SIZE_LIMITS:
		{
			if ( fWinBorder != NULL )
			{
				BPoint cMinSize(0.0f,0.0f);
				BPoint cMaxSize(FLT_MAX,FLT_MAX);
				msg->FindPoint( "min_size", &cMinSize );
				msg->FindPoint( "max_size", &cMaxSize );
				fWinBorder->SetSizeLimits( cMinSize, cMaxSize );
			}
			break;
		}
		case AS_ACTIVATE_WINDOW:
		{
			bool bFocus = true;

			if ( msg->FindBool( "focus", &bFocus ) < 0 )
			{
				printf( "Error: AS_ACTIVATE_WINDOW message have no 'focus' field\n" );
				break;
			}
			g_cLayerGate.Close();
			SetFocus( bFocus );
			g_cLayerGate.Open();
			break;
		}
		case WR_TOGGLE_VIEW_DEPTH:
		{
			Layer* pcView;
			int32    hView = -1;

			msg->FindInt32( "view", &hView );

			g_cLayerGate.Close();
			pcView = FindLayer( hView );

			if ( pcView != NULL  )
			{
				Layer* pcParent = pcView->GetParent();
				if ( pcParent != NULL )
				{
					if ( fWinBorder != NULL && pcView == fTopLayer )
					{
						pcView = fWinBorder;
						pcView->ToggleDepth();
						pcParent->UpdateRegions( false );
					}
				}
			}
			ServerWindow::HandleMouseTransaction();
			g_cLayerGate.Open();

			BMessage cReply;
			msg->SendReply( &cReply );
			break;
		}
		case WR_BEGIN_DRAG:
		{
			ServerBitmap*  pcBitmap = NULL;
			int32         hBitmap = -1;
			BRect       cBounds(0,0,0,0);
			BPoint      cHotSpot(0,0);

			if ( msg->FindInt32( "bitmap", &hBitmap ) < 0 )
			{
				printf( "No 'bitmap' field in WR_BEGIN_DRAG message\n" );
				break;
			}
			
			if ( msg->FindPoint( "hot_spot", &cHotSpot ) < 0 )
			{
				printf( "No 'hot_spot' field in WR_BEGIN_DRAG message\n" );
				break;
			}

			if ( hBitmap != -1 )
			{
				pcBitmap = fServerApp->FindBitmap(hBitmap);
				if ( pcBitmap == NULL )
				{
					printf( "Error: Attempt to start drag operation with invalid bitmap %ld\n", hBitmap );
					break;
				}
			}
			else
			{
				if ( msg->FindRect( "bounds", &cBounds ) < 0 )
				{
					printf( "No 'bounds' field in WR_BEGIN_DRAG message\n" );
					break;
				}
			}

			g_cLayerGate.Close();
			if ( s_bIsDragging == false ) {
				s_pcDragSprite = new SrvSprite( static_cast<IRect>(cBounds),
												IPoint(desktop->GetDisplayDriver()->GetCursorPosition()),
												IPoint(cHotSpot),
												g_pcTopView->GetBitmap(), pcBitmap );
				msg->FindMessage( "data", &s_cDragMessage );
				s_cDragMessage.AddPoint( "_hot_spot", cHotSpot );
				s_bIsDragging = true;
			}
			g_cLayerGate.Open();
			BMessage cReply;
			msg->SendReply( &cReply );
			break;
		}
		case WR_GET_MOUSE:
		{
			BMessage cReply;
			cReply.AddPoint( "position", desktop->GetDisplayDriver()->GetCursorPosition() );
			//cReply.AddInt32( "buttons", AppServer::GetMouseButtons() );
			msg->SendReply( &cReply );
			break;
		}
		case WR_SET_MOUSE_POS:
		{
			BPoint cPos;
			if ( msg->FindPoint( "position", &cPos ) == 0 )
			{
			//	InputNode::SetMousePos( cPos );
			}
			break;
		}
		default:
		{
			printf("ServerWindow %s received unexpected code\n",fTitle.String());
			break;
		}
	}
	return true;
}

void ServerWindow::NotifyWindowFontChanged( bool bToolWindow )
{
	if ( m_pcDecorator == NULL )
	{
		return;
	}

	BRect cFrame = Frame();
	m_pcDecorator->FontChanged();
	SetFrame( cFrame );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ServerWindow::ServerWindow( ServerApp* pcApp, ServerBitmap* pcBitmap ) : fLocker( "swindow_lock" )
{
	fServerApp        = pcApp;
	fClientWinPort   = -1;
	_receiver     = -1;

	_appTarget  = NULL;
	fFlags       = 0;
	fLook = B_NO_BORDER_WINDOW_LOOK;
	fFeel = B_NORMAL_WINDOW_FEEL;
	m_bOffscreen   = true;
	fWorkspaces = ~0;

	m_bBorderHit   = false;

	fTopLayer = new Layer( BRect( 0, 0, pcBitmap->Width() - 1, pcBitmap->Height() - 1 ),
							 "bitmap_layer", NULL, 0, NULL, NULL );
	fTopLayer->SetWindow( this );
	fWinBorder = NULL;
	m_pcDecorator = NULL;

	SetBitmap( pcBitmap );
	fMonitorThreadID = -1;
}




void ServerWindow::SendMessageToClient( BMessage* pcMsg ) const
{
	if ( _appTarget != NULL )
	{
		_appTarget->SendMessage( pcMsg );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Layer* ServerWindow::GetTopView() const
{
	return( fWinBorder );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerWindow::SetDesktopMask( uint32 nMask )
{
	RemoveWindowFromDesktop( this );
	fWorkspaces = nMask;
	AddWindowToDesktop( this );
}


bool ServerWindow::HasPendingSizeEvents( Layer* pcLayer )
{
	return( fWinBorder != NULL && pcLayer != fWinBorder && fWinBorder->HasPendingSizeEvents() );
}


/*!
	\brief Iterator for graphics update messages
	\param msgbuffer Buffer containing the graphics message
*/
void ServerWindow::DispatchGraphicsMessage(WR_Render_s* msgbuffer)
{
	bool        bViewsMoved = false;
	
	g_cLayerGate.Lock();
	
	int    nLowest = INT_MAX;
	Layer* pcLowestLayer = NULL;
	uint   nBufPos = 0;

	for ( int i = 0 ; i < msgbuffer->nCount ; ++i )
	{
		GRndHeader_s* psHdr = (GRndHeader_s*) &msgbuffer->aBuffer[ nBufPos ];
		nBufPos += psHdr->nSize;
		if ( nBufPos > RENDER_BUFFER_SIZE )
		{
			printf( "Error: ServerWindow::DispatchGraphicsMessage() invalid message size %lu\n", psHdr->nSize );
			break;
		}

		if ( bViewsMoved && (psHdr->nCmd != DRC_SET_FRAME || psHdr->nCmd != DRC_SHOW_VIEW) )
		{
			bViewsMoved = false;

			if ( pcLowestLayer != NULL )
			{
				g_cLayerGate.Unlock();
				g_cLayerGate.Close();
				SrvSprite::Hide( /*pcLowestLayer->ConvertToRoot( pcLowestLayer->Bounds() )*/ );
				pcLowestLayer->UpdateRegions( false );
				ServerWindow::HandleMouseTransaction();
				SrvSprite::Unhide();
				nLowest = INT_MAX;
				g_cLayerGate.Open();
				g_cLayerGate.Lock();
			}
			bViewsMoved = false;
		}

		// Determine current layer from view token
		Layer* cl = FindLayer( psHdr->hViewToken );

		if ( cl == NULL  )
		{
			continue;
		}

		switch( psHdr->nCmd )
		{
			case AS_SET_HIGH_COLOR:
			{
				GRndSetrgb_color* psMsg = (GRndSetrgb_color*) psHdr;
				cl->fLayerData->highcolor.SetColor(psMsg->sColor);
				break;
			}
			case AS_SET_LOW_COLOR:
			{
				GRndSetrgb_color* psMsg = (GRndSetrgb_color*) psHdr;
				cl->fLayerData->lowcolor.SetColor(psMsg->sColor);
				break;
			}
			case AS_SET_VIEW_COLOR:
			{
				GRndSetrgb_color* psMsg = (GRndSetrgb_color*) psHdr;
				cl->fLayerData->viewcolor.SetColor(psMsg->sColor);
				break;
			}
			case DRC_BEGIN_UPDATE:
				cl->BeginUpdate();
				break;

			case DRC_END_UPDATE:
				cl->EndUpdate();
				break;

			case DRC_LINE32:
			{
				GRndLine32_s* psMsg = (GRndLine32_s*) psHdr;
				cl->DrawLine( psMsg->sToPos );
				break;
			}

			case DRC_FILL_RECT32:
			{
				GRndRect32_s* psMsg = (GRndRect32_s*) psHdr;
				cl->FillRect( psMsg->sRect, psMsg->sColor );
				break;
			}

			case DRC_STROKE_RECT32:
			{
				GRndRect32_s* psMsg = (GRndRect32_s*) psHdr;
				cl->StrokeRect( psMsg->sRect, psMsg->sColor );
				break;
			}
			
			case DRC_FILL_ELLIPSE:
			{
				GRndRect32_s* psMsg = (GRndRect32_s*) psHdr;
				cl->FillEllipse( psMsg->sRect, psMsg->sColor );
				break;
			}

			case DRC_STROKE_ELLIPSE:
			{
				GRndRect32_s* psMsg = (GRndRect32_s*) psHdr;
				cl->StrokeEllipse( psMsg->sRect, psMsg->sColor );
				break;
			}
			
			case DRC_COPY_RECT:
				if ( NULL != GetBitmap() )
				{
					g_cLayerGate.Unlock();
					g_cLayerGate.Close();
					cl->CopyRect( GetBitmap(), (GRndCopyRect_s*) psHdr );
					g_cLayerGate.Open();
					g_cLayerGate.Lock();
				}
				break;

			case DRC_DRAW_BITMAP:
				if ( NULL != GetBitmap() )
				{
					GRndDrawBitmap_s* psReq = (GRndDrawBitmap_s*) psHdr;
					ServerBitmap* sb = fServerApp->FindBitmap(psReq->hBitmapToken);
					if ( NULL != sb )
					{
						cl->DrawBitMap( GetBitmap(), sb, psReq->cSrcRect, psReq->cDstRect.LeftTop() );
					}
					else
					{
						printf("ERROR: ServerBitmap not found!\n");
					}
				}
				break;

			case DRC_SET_COLOR32:
			{
				GRndSetrgb_color* psMsg = (GRndSetrgb_color*) psHdr;
				switch( psMsg->nWhichPen )
				{
					case PEN_HIGH:  cl->SetHighColor(psMsg->sColor);  break;
					case PEN_LOW:   cl->SetLowColor(psMsg->sColor);   break;
					case PEN_ERASE: cl->SetEraseColor(psMsg->sColor); break;
				}
				break;
			}

			case DRC_SET_PEN_POS:
			{
				GRndSetPenPos_s* psMsg = (GRndSetPenPos_s*) psHdr;
				if ( psMsg->bRelative )
				{
					cl->MovePenBy( BPoint( psMsg->sPosition ) );
				}
				else
				{
					cl->MovePenTo( BPoint( psMsg->sPosition ) );
				}
				break;
			}

			case DRC_SET_FONT:
			{
				GRndSetFont_s* psMsg  = (GRndSetFont_s*) psHdr;
				g_cLayerGate.Unlock();
				g_cLayerGate.Close();
				cl->SetFont(fServerApp->GetFont( psMsg->hFontID ) );
				g_cLayerGate.Open();
				g_cLayerGate.Lock();
				break;
			}

			case DRC_DRAW_STRING:
			{
				GRndDrawString_s* psMsg = (GRndDrawString_s*) psHdr;
				cl->DrawString( psMsg->zString, psMsg->nLength  );
				break;
			}

			case DRC_SET_FRAME:
			{
				GRndSetFrame_s* psMsg = static_cast<GRndSetFrame_s*>(psHdr);

				if ( fWinBorder != NULL && cl == fTopLayer )
				{
					Layer* pcParent = fWinBorder->GetParent();
					if ( pcParent != NULL )
					{ // Can't move windows not attached to a desktop
						SetFrame( psMsg->cFrame );
						nLowest = 0;
						pcLowestLayer = pcParent;
					}
				}
				else
				{
					Layer* pcParent = cl->GetParent();
					if ( pcParent != NULL ) {
						cl->SetFrame( psMsg->cFrame.OffsetByCopy(pcParent->GetScrollOffset()) );
						if ( cl->GetLevel() < nLowest )
						{
							nLowest = cl->GetLevel();
							pcLowestLayer = cl->GetParent();
						}
					}
				}
				bViewsMoved = true;
				break;
			}

			case DRC_SHOW_VIEW:
			{
				GRndShowView_s* psMsg = static_cast<GRndShowView_s*>(psHdr);

				if ( fWinBorder != NULL && cl == fTopLayer )
				{
					if (psMsg->bVisible)
						fWinBorder->Show();
					else
						fWinBorder->Hide();

					if ( psMsg->bVisible )
					{
						fWinBorder->MakeTopChild();
					}
					Layer* pcParent = fWinBorder->GetParent();
					if ( pcParent != NULL )
					{ // Can't show not attached to a desktop
						nLowest = 0;
						pcLowestLayer = pcParent;
					}
				}
				else
				{
					Layer* pcParent = cl->GetParent();
					if ( pcParent != NULL )
					{
						if (psMsg->bVisible)
							cl->Show();
						else
							cl->Hide();

						if ( pcParent->GetLevel() < nLowest )
						{
							nLowest = pcParent->GetLevel();
							pcLowestLayer = pcParent;
						}
					}
				}
				bViewsMoved = true;
				break;
			}

			case DRC_SCROLL_VIEW:
			{
				GRndScrollView_s* psMsg = static_cast<GRndScrollView_s*>(psHdr);

				g_cLayerGate.Unlock();
				g_cLayerGate.Close();
				SrvSprite::Hide( static_cast<IRect>(cl->ConvertToTop( cl->Bounds() )) );
				cl->ScrollBy( psMsg->cDelta );
				SrvSprite::Unhide();
				g_cLayerGate.Open();
				g_cLayerGate.Lock();
				break;
			}
			case DRC_SET_DRAWING_MODE:
				cl->fLayerData->draw_mode = ( (drawing_mode) static_cast<GRndSetDrawingMode_s*>(psHdr)->nDrawingMode );
				break;

			case DRC_INVALIDATE_RECT:
			{
				IRect cRect( static_cast<GRndInvalidateRect_s*>(psHdr)->m_cRect );
				cl->Invalidate( cRect.AsBRect().OffsetByCopy(cl->m_cScrollOffset)/*, static_cast<GRndInvalidateRect_s*>(psHdr)->m_bRecurse*/ );
				if ( cl->GetLevel() < nLowest )
				{
					nLowest = cl->GetLevel();
					pcLowestLayer = cl;
				}
				bViewsMoved = true;
				break;
			}
			
			case DRC_INVALIDATE_VIEW:
				cl->Invalidate( static_cast<GRndInvalidateView_s*>(psHdr)->m_bRecurse );
				if ( cl->GetLevel() < nLowest )
				{
					nLowest = cl->GetLevel();
					pcLowestLayer = cl;
				}
				bViewsMoved = true;
				break;
		}
//    g_cLayerGate.Unlock();
	}
	g_cLayerGate.Unlock();

	if ( bViewsMoved )
	{
		bViewsMoved = false;
		if ( pcLowestLayer != NULL )
		{
			g_cLayerGate.Close();
			SrvSprite::Hide();
			pcLowestLayer->UpdateRegions( false );
			ServerWindow::HandleMouseTransaction();
			SrvSprite::Unhide();
			nLowest = INT_MAX;
			g_cLayerGate.Open();
		}
	}
	
	if ( msgbuffer->hReply != -1 )
	{
		write_port( msgbuffer->hReply, 0, NULL, 0 );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerWindow::SendKbdEvent( int nKeyCode, uint32 nQualifiers, const char* pzConvString, const char* pzRawString )
{
	g_cLayerGate.Lock();
	ServerWindow* pcWnd = GetActiveWindow(false);
	if ( pcWnd != NULL )
	{
		BMessage cMsg( (nKeyCode & 0x80) ? B_KEY_UP : B_KEY_DOWN );

		cMsg.AddPointer( "_widget", pcWnd->fTopLayer->GetUserObject() );
		cMsg.AddInt32( "_raw_key", nKeyCode );
		cMsg.AddInt32( "modifiers", nQualifiers );
		cMsg.AddString( "_string", pzConvString );
		cMsg.AddString( "_raw_string", pzRawString );

		pcWnd->SendMessageToClient( &cMsg );
	}
	g_cLayerGate.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerWindow::MouseMoved( BMessage* pcEvent, int nTransit )
{
	__assertw( NULL != fWinBorder );
	__assertw( NULL != fTopLayer );

	if ( /*m_bBorderHit &&*/ (fFlags & WND_SYSTEM) == 0 )
	{
		BPoint cNewPos;
		pcEvent->FindPoint( "where", &cNewPos );
		if ( fWinBorder->MouseMoved( _appTarget, cNewPos - fWinBorder->GetLeftTop(), nTransit ) )
		{
			return;
		}
	}

	BMessage cMsg( *pcEvent );

	cMsg.AddPointer( "_widget", fTopLayer->GetUserObject() );
	cMsg.AddInt32( "_transit", nTransit );

	if ( s_bIsDragging )
	{
		cMsg.AddMessage( "_drag_message", &s_cDragMessage );
	}

	if ( _appTarget != NULL )
	{
		if ( _appTarget->SendMessage( &cMsg ) < 0 )
		{
			printf( "Error: ServerWindow::MouseMoved() failed to send message to %s\n", fTitle.String() );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerWindow::MouseDown( BMessage* pcEvent )
{
	BPoint  cPos;
	__assertw( NULL != fWinBorder );
	__assertw( NULL != fTopLayer );

	pcEvent->FindPoint( "where", &cPos );

	if ( fTopLayer->Frame().Contains( cPos - fWinBorder->GetLeftTop() ) == false && (fFlags & WND_SYSTEM) == 0 )
	{
		int32 nButton;
		pcEvent->FindInt32( "buttons", &nButton );
		m_bBorderHit = true;
		if ( fWinBorder->MouseDown( _appTarget, cPos - fWinBorder->GetLeftTop(), nButton ) )
		{
			s_pcDragWindow = this;
			return;
		}
	}

	BMessage cMsg( *pcEvent );

	cMsg.AddPointer( "_widget", fTopLayer->GetUserObject() );

	if ( _appTarget != NULL )
	{
		if ( _appTarget->SendMessage( &cMsg ) < 0 )
		{
			printf( "Error: ServerWindow::MouseDown() failed to send message to %s\n", fTitle.String() );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerWindow::MouseUp( BMessage* pcEvent, bool bSendDragMsg )
{
	__assertw( NULL != fWinBorder );
	//__assertw( NULL != fTopLayer );

	BMessage cMsg( *pcEvent );

	cMsg.AddPointer( "_widget", fTopLayer->GetUserObject() );
	if ( bSendDragMsg && s_bIsDragging ) {
		cMsg.AddMessage( "_drag_message", &s_cDragMessage );
	}
	if ( _appTarget != NULL )
	{
		if ( _appTarget->SendMessage( &cMsg ) < 0 )
		{
			printf( "Error: ServerWindow::MouseUp() failed to send message to %s\n", fTitle.String() );
		}
	}
}


void ServerWindow::HandleMouseMoved( const BPoint& cMousePos, BMessage* pcEvent )
{
	ServerWindow* pcActiveWindow = GetActiveWindow(false);
	ServerWindow* pcMouseWnd = NULL;

	if ( s_pcDragWindow != NULL )
	{
		s_pcDragWindow->fWinBorder->MouseMoved( s_pcDragWindow->_appTarget,
												cMousePos - s_pcDragWindow->fWinBorder->GetLeftTop(), B_INSIDE_VIEW );
		return;
	}

	if ( pcActiveWindow == NULL || (pcActiveWindow->Flags() & WND_SYSTEM) == 0 )
	{
		Layer* pcView = g_pcTopView->GetChildAt( cMousePos );

		if ( pcView != NULL )
		{
			pcMouseWnd = pcView->GetWindow();
		}
		else
		{
			pcMouseWnd = NULL;
		}
	}
	if ( s_pcLastMouseWindow != pcMouseWnd )
	{
		if ( s_pcLastMouseWindow != NULL )
		{
			// Give the window a chance to deliver a B_EXITED_VIEW to it's views
			s_pcLastMouseWindow->MouseMoved( pcEvent, B_EXITED_VIEW );
			if ( s_pcLastMouseWindow == pcActiveWindow )
			{
				pcActiveWindow = NULL;
			}
		}

		if ( pcMouseWnd != NULL )
		{
			pcMouseWnd->MouseMoved( pcEvent, B_ENTERED_VIEW );
			if ( pcMouseWnd == pcActiveWindow )
			{
				pcActiveWindow = NULL;
			}
		}
		s_pcLastMouseWindow = pcMouseWnd;
	}
	else
	{
		if ( pcMouseWnd != NULL )
		{
			pcMouseWnd->MouseMoved( pcEvent, B_INSIDE_VIEW );
			if ( pcMouseWnd == pcActiveWindow )
			{
				pcActiveWindow = NULL;
			}
		}
	}
	if ( pcActiveWindow != NULL )
	{
		pcActiveWindow->MouseMoved( pcEvent, B_OUTSIDE_VIEW );
	}
	
	if ( pcMouseWnd != NULL && pcMouseWnd != pcActiveWindow )
	{
		pcMouseWnd->fWinBorder->MouseMoved( pcMouseWnd->_appTarget, cMousePos - pcMouseWnd->fWinBorder->GetLeftTop(), B_INSIDE_VIEW );
	}
	
	if ( s_bIsDragging && s_pcDragSprite != NULL )
	{
		s_pcDragSprite->MoveTo( IPoint(cMousePos) );
	}
}


void ServerWindow::HandleMouseDown( const BPoint& cMousePos, int nButton, BMessage* pcEvent )
{
	ServerWindow* pcActiveWindow = GetActiveWindow(false);
	ServerWindow* pcMouseWnd = NULL;

	if ( pcActiveWindow == NULL || (pcActiveWindow->Flags() & WND_SYSTEM) == 0 )
	{
		pcMouseWnd = s_pcLastMouseWindow;
	}

	if ( nButton == 1 )
	{
		if ( pcMouseWnd != NULL )
		{
			bigtime_t nCurTime;
			
			if ( pcEvent->FindInt64( "when", &nCurTime ) != 0 )
			{
				printf( "Error: ServerWindow::HandleInputEvent() B_MOUSE_DOWN message has no when field\n" );
				nCurTime = system_time();
			}
			pcMouseWnd->SetFocus( true );
			if ( pcMouseWnd->fTopLayer->Frame().Contains( cMousePos - pcMouseWnd->fWinBorder->Frame().LeftTop() ) == false )
			{
				int32 nQualifiers;

				if ( pcEvent->FindInt32( "modifiers", &nQualifiers ) != 0 )
				{
					printf( "Error: ServerWindow::HandleInputEvent() B_MOUSE_DOWN message has no modifiers field\n" );
					nQualifiers = desktop->GetDisplayDriver()->GetQualifiers();
				}

				AppserverConfig* pcConfig = AppserverConfig::GetInstance();
				if (!(nQualifiers & B_CONTROL_KEY) || pcMouseWnd->m_nLastHitTime + pcConfig->GetDoubleClickTime() > nCurTime)
				{
					pcMouseWnd->fWinBorder->MakeTopChild();
					pcMouseWnd->fWinBorder->GetParent()->UpdateRegions( false );
					ServerWindow::HandleMouseTransaction();
				}
			}
			pcMouseWnd->m_nLastHitTime = nCurTime;
		}
		else if ( GetActiveWindow(true) != NULL )
		{
			GetActiveWindow(true)->SetFocus( false );
		}
	}

	pcActiveWindow = GetActiveWindow(false);
	if ( pcActiveWindow != NULL )
	{
		pcActiveWindow->MouseDown( pcEvent );
	}
}


void ServerWindow::HandleMouseUp( const BPoint& cMousePos, int nButton, BMessage* pcEvent )
{
	if ( s_pcDragWindow != NULL )
	{
		BPoint cPos;
		int32 nButton;
		pcEvent->FindPoint( "where", &cPos );
		pcEvent->FindInt32( "buttons", &nButton );
		s_pcDragWindow->fWinBorder->MouseUp( s_pcDragWindow->_appTarget, cPos - s_pcDragWindow->fWinBorder->GetLeftTop(), nButton );
		s_pcDragWindow = NULL;
		return;
	}

	ServerWindow* pcActiveWindow = GetActiveWindow(false);
	ServerWindow* pcMouseWnd = NULL;

	if ( pcActiveWindow == NULL || (pcActiveWindow->Flags() & WND_SYSTEM) == 0 )
	{
		pcMouseWnd = s_pcLastMouseWindow;
	}

	if ( pcMouseWnd != NULL )
	{
		pcMouseWnd->MouseUp( pcEvent, true );
	}
	
	if ( pcActiveWindow != NULL && pcMouseWnd != pcActiveWindow ) {
		pcActiveWindow->MouseUp( pcEvent, false );
	}
	
	if ( s_bIsDragging )
	{
		SrvSprite::Hide();
		delete s_pcDragSprite;
		s_cDragMessage.MakeEmpty();
		s_bIsDragging = false;
		SrvSprite::Unhide();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerWindow::HandleInputEvent( BMessage* pcEvent )
{
	g_cLayerGate.Close();
	BPoint cMousePos;

	pcEvent->FindPoint( "where", &cMousePos );

	if ( pcEvent->what == B_MOUSE_MOVED )
	{
		HandleMouseMoved( cMousePos, pcEvent );
		g_cLayerGate.Open();
		return;
	}
	else if ( pcEvent->what == B_MOUSE_DOWN )
	{
		int32 nButton;
		if ( pcEvent->FindInt32( "buttons", &nButton ) != 0 )
		{
			printf( "Error: ServerWindow::HandleInputEvent() B_MOUSE_DOWN message has no buttons field\n" );
		}
		HandleMouseDown( cMousePos, nButton, pcEvent );
		g_cLayerGate.Open();
		return;
	}
	else if ( pcEvent->what == B_MOUSE_UP )
	{
		int32 nButton;
		if ( pcEvent->FindInt32( "buttons", &nButton ) != 0 )
		{
			printf( "Error: ServerWindow::HandleInputEvent() B_MOUSE_UP message has no buttons field\n" );
		}
		HandleMouseUp( cMousePos, nButton, pcEvent );
		g_cLayerGate.Open();
		return;
	}
	g_cLayerGate.Open();
}

void ServerWindow::HandleMouseTransaction()
{
	BPoint cMousePos = desktop->GetDisplayDriver()->GetCursorPosition();
	Layer* pcView = g_pcTopView->GetChildAt( cMousePos );
	ServerWindow* pcWnd = (pcView != NULL) ? pcView->GetWindow() : NULL;

	if ( pcWnd != s_pcLastMouseWindow )
	{
		//InputNode::SetMousePos( cMousePos );
		
		//BMessage* pcEvent = new BMessage( B_MOUSE_MOVED );
		//pcEvent->AddPoint( "delta_move", cMousePos - AppServer::s_cMousePos );
		//app_server->EnqueueEvent(pcEvent, 0 /*GetQualifiers()*/);
		
		// TODO: force a B_MOUSE_MOVED event to trigger any cursor changes
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerWindow::BeginRegionUpdate()
{
	m_bNeedRegionUpdate = true;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ServerWindow::DispatchMessage(const void* psMsg, int nCode)
{
	bool bDoLoop = true;

	switch( nCode )
	{
		case AS_LAYER_CREATE:
		case AS_LAYER_DELETE:
		case AS_WINDOW_TITLE:
		case AS_SET_FLAGS:
		case AS_SET_SIZE_LIMITS:
		case AS_SET_ALIGNMENT:
		case AS_ACTIVATE_WINDOW:
		case WR_TOGGLE_VIEW_DEPTH:
		case WR_BEGIN_DRAG:
		case WR_GET_MOUSE:
		case WR_SET_MOUSE_POS:
		{
			try {
				BMessage cReq( psMsg, true );
				DispatchMessage( &cReq );
			} catch (...) {
				printf( "Error: DispatchMessage() caught exception while unflatting message\n" );
			}
			break;
		}

		case WR_RENDER:
			DispatchGraphicsMessage( (WR_Render_s*) psMsg );
			break;

		case WR_WND_MOVE_REPLY:
			if ( fWinBorder != NULL )
			{
				fWinBorder->WndMoveReply( _appTarget );
			}
			break;

		case WR_GET_PEN_POSITION:
		{
			WR_GetPenPosition_s* psReq = static_cast<WR_GetPenPosition_s*>((void*)psMsg);
			WR_GetPenPositionReply_s sReply;

			Layer* pcView = FindLayer( psReq->m_hViewHandle );

			if ( pcView != NULL )
			{
				sReply.m_cPos = pcView->GetPenPosition();
			}
			else
			{
				printf( "Error: ServerWindow::DispatchMessage() request for pen position in invalid view %08x\n", psReq->m_hViewHandle );
			}
			write_port( psReq->m_hReply, 0, &sReply, sizeof( sReply ) );
			break;
		}
		case WR_UPDATE_REGIONS:
			g_cLayerGate.Close();
			if ( fWinBorder != NULL )
			{
				fWinBorder->UpdateRegions();
			}
			else
			{
				fTopLayer->UpdateRegions();
			}
			g_cLayerGate.Open();
			break;
		case B_QUIT_REQUESTED:
		case AS_QUIT_WINDOW:
		{
			bDoLoop = false;
			delete _appTarget;
			_appTarget = NULL;
			break;
		}
		default:
			printf( "Warning : ServerWindow::DispatchMessage() Unknown command %d\n", nCode );
			break;
	}
	return( bDoLoop );
}


/*!
	\brief Message-dispatching loop for the ServerWindow

	MonitorWin() watches the ServerWindow's message port and dispatches as necessary
	\param data The thread's ServerWindow
	\return Throwaway code. Always 0.
*/
int32 ServerWindow::MonitorWin(void *data)
{
	ServerWindow 	*win = (ServerWindow *)data;
	void*        psMsg        = new char[8192];
	bool			quitting = false;
	int32			code;
	
	while(!quitting)
	{
		if ( read_port( win->_receiver, &code, psMsg, 8192 ) >= 0 )
		{
			if ( win->m_bNeedRegionUpdate )
			{
				g_cLayerGate.Lock();
				if ( win->fWinBorder != NULL )
				{
					win->fWinBorder->UpdateRegions();
				}
				else
				{
					win->fTopLayer->UpdateRegions();
				}
				win->m_bNeedRegionUpdate = false;
				g_cLayerGate.Unlock();
			}

			if (code == 'pjpp')
			{
				BMessage temp(psMsg, true);
				if (!win->DispatchMessage( psMsg, temp.what ))
					break;
			}
			else
			{
				if (!win->DispatchMessage( psMsg, code ))
					break;
			}
		}
	}
	delete[] (char*)psMsg;

	ServerApp*        pcApp = win->App();

	__assertw( NULL != pcApp );

	AR_DeleteWindow_s sReq;

	sReq.pcWindow        = win;

	write_port( pcApp->_receiver, AS_DELETE_WINDOW, &sReq, sizeof(sReq) );
	exit_thread( 0 );
	return( 0 );
}


