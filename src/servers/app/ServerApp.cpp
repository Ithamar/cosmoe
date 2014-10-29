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
//	File Name:		ServerApp.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Server-side BApplication counterpart
//  
//------------------------------------------------------------------------------

#include <SysCursor.h>


#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <errno.h>

#include <OS.h>
#include <kernel.h>
#include <Messenger.h>
#include <signal.h>

#include <ServerProtocol.h>

#include "AppServer.h"
#include "ServerFont.h"
#include "FontFamily.h"
#include "sprite.h"
#include "config.h"
#include "BitmapManager.h"
#include "CursorManager.h"
#include "Desktop.h"
#include "FontServer.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "ServerCursor.h"
#include "ServerBitmap.h"


#include <macros.h>

ServerApp*                           ServerApp::s_pcFirstApp = NULL;
BLocker                              ServerApp::s_cAppListLock( "app_list" );

//#define DEBUG_SERVERAPP

#ifdef DEBUG_SERVERAPP
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

/*!
	\brief Constructor
	\param sendport port ID for the BApplication which will receive the ServerApp's messages
	\param rcvport port by which the ServerApp will receive messages from its BApplication.
	\param fSignature NULL-terminated string which contains the BApplication's
	MIME fSignature.
*/
ServerApp::ServerApp(port_id hEventPort,
		team_id clientTeamID, int32 handlerID, const char *signature)
    : m_cFontLock( "app_font_lock" )
{
	// it will be of *very* musch use in correct window order
	fClientTeamID		= clientTeamID;

	// need to copy the fSignature because the message buffer
	// owns the copy which we are passed as a parameter.
	fSignature=(signature)?signature:"application/x-vnd.NULL-application-signature";

	// token ID of the BApplication's BHandler object. Used for BMessage target specification
	fHandlerToken=handlerID;

	s_cAppListLock.Lock();
	m_pcNext = s_pcFirstApp;
	s_pcFirstApp = this;
	s_cAppListLock.Unlock();

	m_pcAppTarget = new BMessenger( -1, hEventPort, -1, false );

	_receiver = create_port( 15, "srvapp" );

	fSWindowList=new BList(0);
	fBitmapList=new BList(0);
	fPictureList=new BList(0);
	fIsActive=false;

	ServerCursor *defaultc=cursormanager->GetCursor(B_CURSOR_DEFAULT);
	
	fAppCursor=(defaultc)?new ServerCursor(defaultc):NULL;
	fAppCursor->SetOwningTeam(fClientTeamID);
	fLockSem=create_sem(1,"ServerApp sem");

	// Does this even belong here any more? --DW
//	_driver=desktop->GetDisplayDriver();

	fCursorHidden=false;

	Run();

	STRACE(("ServerApp %s:\n",fSignature.String()));
}

//! Does all necessary teardown for application
ServerApp::~ServerApp(void)
{
	STRACE(("*ServerApp %s:~ServerApp()\n",fSignature.String()));
	int32 i;

	ServerApp** ppcTmp;

	if ( m_cFonts.size() > 0  )
	{
		printf( "Application %s forgot to delete %d fonts!\n", fSignature.String(), m_cFonts.size() );
	}
	fSWindowList->MakeEmpty();
	delete fSWindowList;

	ServerBitmap *tempbmp;
	for(i=0;i<fBitmapList->CountItems();i++)
	{
		tempbmp=(ServerBitmap*)fBitmapList->ItemAt(i);
		if(tempbmp)
			delete tempbmp;
	}
	fBitmapList->MakeEmpty();
	delete fBitmapList;

	fPictureList->MakeEmpty();
	delete fPictureList;

	if(fAppCursor)
		delete fAppCursor;

	if ( m_cSprites.size() > 0  )
	{
		printf( "Application %s forgot to delete %d sprites!\n", fSignature.String(), m_cSprites.size() );
	}

	while( m_cFonts.empty() == false )
	{
		std::set<FontNode*>::iterator i = m_cFonts.begin();
		FontNode* pcNode = (*i);
		pcNode->Release();
		m_cFonts.erase( i );
	}

	while( m_cSprites.empty() == false )
	{
		std::set<SrvSprite*>::iterator i = m_cSprites.begin();
		SrvSprite* pcSprite = (*i);
		delete pcSprite;
		m_cSprites.erase( i );
	}

	s_cAppListLock.Lock();
	for ( ppcTmp = &s_pcFirstApp ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->m_pcNext )
	{
		if ( *ppcTmp == this )
		{
			*ppcTmp = m_pcNext;
			break;
		}
	}
	s_cAppListLock.Unlock();

	cursormanager->RemoveAppCursors(fClientTeamID);
	delete_sem(fLockSem);
	
	STRACE(("#ServerApp %s:~ServerApp()\n",fSignature.String()));

	delete_port( _receiver );
}


/*!
	\brief Starts the ServerApp monitoring for messages
	\return false if the application couldn't start, true if everything went OK.
*/
bool ServerApp::Run(void)
{
	// Unlike a BApplication, a ServerApp is *supposed* to return immediately
	// when its Run() function is called.
	fMonitorThreadID=spawn_thread(MonitorApp,fSignature.String(),B_NORMAL_PRIORITY,this);
	if(fMonitorThreadID==B_NO_MORE_THREADS || fMonitorThreadID==B_NO_MEMORY)
		return false;

	resume_thread(fMonitorThreadID);
	return true;
}

/*!
	\brief Pings the target app to make sure it's still working
	\return true if target is still "alive" and false if "He's dead, Jim." 
	"But that's impossible..."
	
	This function is called by the app_server thread to ensure that
	the target app still exists. We do this not by sending a message
	but by calling get_port_info. We don't want to send ping messages
	just because the app might simply be hung. If this is the case, it
	should be up to the user to kill it. If the app has been killed, its
	ports will be invalid. Thus, if get_port_info returns an error, we
	tell the app_server to delete the respective ServerApp.
*/
bool ServerApp::PingTarget(void)
{
	return true;
}

/*!
	\brief Send a message to the ServerApp's BApplication
	\param msg The message to send
*/
void ServerApp::SendMessageToClient(BMessage *msg) const
{
	if ( m_pcAppTarget != NULL )
	{
		m_pcAppTarget->SendMessage(msg);
	}
}


/*!
	\brief Sets the ServerApp's active status
	\param value The new status of the ServerApp.
	
	This changes an internal flag and also sets the current cursor to the one specified by
	the application
*/
void ServerApp::Activate(bool value)
{
	fIsActive=value;
	SetAppCursor();
}
 
//! Sets the cursor to the application cursor, if any.
void ServerApp::SetAppCursor(void)
{
	if(fAppCursor)
		cursormanager->SetCursor(fAppCursor->ID());
	else
		cursormanager->SetCursor(B_CURSOR_DEFAULT);
}


void ServerApp::Lock(void)
{
	acquire_sem(fLockSem);
}

void ServerApp::Unlock(void)
{
	release_sem(fLockSem);
}

/*!
	\brief The thread function ServerApps use to monitor messages
	\param data Pointer to the thread's ServerApp object
	\return Throwaway value - always 0
*/
int32 ServerApp::MonitorApp(void *data)
{
	// Message-dispatching loop for the ServerApp

	ServerApp *app = (ServerApp *)data;
	char *msg     = new char[8192];
	
	bool quitting = false;


	while( !quitting || app->m_cWindows.empty() == false )
	{
		int32        nCode;

		if ( read_port( app->_receiver, &nCode, msg, 8192 ) >= 0 )
		{
			app->Lock();
			
			if (nCode == 'pjpp')
			{
				BMessage temp(msg, true);
				if ( app->_DispatchMessage(msg, temp.what) == false )
				{
					quitting = true;
				}
			}
			else
			{
				if ( app->_DispatchMessage(msg, nCode) == false )
				{
					quitting = true;
				}
			}
			
			app->Unlock();
		}
	}
	delete[] msg;

	delete app;

	exit_thread( 0 );
	return 0;
}

void ServerApp::ReplaceDecorators()
{
	g_cLayerGate.Close();
	s_cAppListLock.Lock();

	for ( ServerApp* pcApp = s_pcFirstApp ; pcApp != NULL ; pcApp = pcApp->m_pcNext )
	{
		std::set<ServerWindow*>::iterator i;

		for ( i = pcApp->m_cWindows.begin() ; i != pcApp->m_cWindows.end() ; ++i )
		{
			(*i)->ReplaceDecorator();
		}
	}

	s_cAppListLock.Unlock();

	g_pcTopView->Invalidate( true );
	g_pcTopView->SetDirtyRegFlags();
	g_pcTopView->UpdateRegions();
	ServerWindow::HandleMouseTransaction();
	g_cLayerGate.Open();
}



void ServerApp::NotifyColorCfgChanged()
{
	s_cAppListLock.Lock();
	for ( ServerApp* pcApp = s_pcFirstApp ; pcApp != NULL ; pcApp = pcApp->m_pcNext )
	{
		__assertw( pcApp->m_pcAppTarget != NULL );
		if ( pcApp->m_pcAppTarget != NULL )
		{
			BMessage cMsg( M_COLOR_CONFIG_CHANGED );

			for ( int i = 0 ; i < COL_COUNT ; ++i )
			{
				cMsg.AddInt32("_colors",
								_get_uint32_color(get_default_color(static_cast<default_color_t>(i)) ) );
			}
			
			if ( pcApp->m_pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				printf( "Error: ServerApp::NotifyColorCfgChanged() failed to send message to app: %s\n", pcApp->fSignature.String() );
			}
		}
	}
	s_cAppListLock.Unlock();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ServerApp* ServerApp::FindApp( team_id hProc )
{
	ServerApp* pcTmp;

	s_cAppListLock.Lock();
	for ( pcTmp = s_pcFirstApp ; pcTmp != NULL ; pcTmp = pcTmp->m_pcNext )
	{
		if ( pcTmp->fClientTeamID == hProc )
		{
			break;
		}
	}
	s_cAppListLock.Unlock();
	return( pcTmp );
}

//----------------------------------------------------------------------------
// NAME:
// DESC: Find all apps with mime signature inMimeSig (or just plain all apps,
//       if inMimeSig is NULL)
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ServerApp::FindApps( BMessage& ioMessage, const char* inMimeSig )
{
	ServerApp* pcTmp;

	s_cAppListLock.Lock();
	for ( pcTmp = s_pcFirstApp ; pcTmp != NULL ; pcTmp = pcTmp->m_pcNext )
	{
		if ( (inMimeSig == NULL) || (strcmp(pcTmp->fSignature.String(), inMimeSig) == 0) )
		{
			ioMessage.AddInt32( "teams", pcTmp->fClientTeamID );
		}
	}
	s_cAppListLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontNode* ServerApp::GetFont( uint32 nID )
{
	FontNode* pcInst = NULL;

	if ( m_cFontLock.Lock() )	// NLS
	{
		std::set<FontNode*>::iterator i = m_cFonts.find( (FontNode*)nID );

		if ( i != m_cFonts.end() )
		{
			pcInst = *i;
		}
		else
		{
			printf( "Error : ServerApp::GetFont() called on invalid font ID %ld\n", nID );
		}
		m_cFontLock.Unlock();
	}
	return( pcInst );
}


/*!
	\brief Handler function for BApplication API messages
	\param code Identifier code for the message. Equivalent to BMessage::what
	\param buffer Any attachments
	
	Note that the buffer's exact format is determined by the particular message. 
	All attachments are placed in the buffer via a PortLink, so it will be a 
	matter of casting and incrementing an index variable to access them.
*/
void ServerApp::_DispatchMessage(BMessage *msg)
{
	switch(msg->what)
	{
#if 0
		case AR_OPEN_BITMAP_WINDOW:
		{
			int   hBitmapHandle;
			void* pUserObj;

			msg->FindInt( "bitmap_handle", &hBitmapHandle );
			msg->FindPointer( "user_obj", &pUserObj );

			g_cLayerGate.Close();
			BitmapNode* pcBmNode = g_pcBitmaps->GetObj( hBitmapHandle );

			if ( pcBmNode == NULL )
			{
				printf( "Error: ServerApp::DispatchMessage() invalid bitmap-handle %d, cant create window\n", hBitmapHandle );
				BMessage cReply;

				cReply.AddInt32( "cmd_port", -1 );
				cReply.AddInt32( "top_view", -1 );
				msg->SendReply( &cReply );

				g_cLayerGate.Open();
				break;
			}

			ServerWindow* pcWindow = new ServerWindow( this, pcBmNode->m_pcBitmap );

			m_cWindows.insert( pcWindow );

			BMessage cReply;

			cReply.AddInt32( "cmd_port", _receiver );
			cReply.AddInt32( "top_view", pcWindow->GetClientView()->GetHandle() );

			msg->SendReply( &cReply );
			g_cLayerGate.Open();
			break;
		}
#endif
		case AS_CREATE_WINDOW:
		{
			// Create the ServerWindow to node monitor a new OBWindow
			
			// Attached data:
			// 2) BRect window frame
			// 3) uint32 window look
			// 4) uint32 window feel
			// 5) uint32 window flags
			// 6) uint32 workspace index
			// 7) int32 BHandler token of the window
			// 8) port_id window's message port
			// 9) const char * title

			BRect frame;
			uint32 look;
			uint32 feel;
			uint32 flags;
			uint32 wkspaces;
			void*   pUserObj;
			port_id	sendPort;
			const char *title;

			msg->FindRect( "frame", &frame );
			msg->FindInt32( "look", (int32*)&look );
			msg->FindInt32( "feel", (int32*)&feel );
			msg->FindInt32( "flags", (int32*)&flags );
			msg->FindInt32( "desktop_mask", (int32*)&wkspaces );
			msg->FindPointer( "user_obj", &pUserObj );
			msg->FindInt32( "event_port", &sendPort );
			msg->FindString( "title", &title );

			STRACE(("ServerApp %s: Got 'New Window' message, trying to do smething...\n",fSignature.String()));

			BMessage cReply;

			g_cLayerGate.Close();

			// ServerWindow constructor will reply with port_id of a newly created port
			ServerWindow *sw = NULL;
			sw = new ServerWindow(frame, title,
				look, feel, pUserObj, flags, this, sendPort, wkspaces);
			sw->Init();
			m_cWindows.insert( sw );
			fSWindowList->AddItem( sw );
			sw->Hide();
			cReply.AddInt32( "top_view", sw->GetClientView()->GetHandle() );
			cReply.AddInt32( "cmd_port", sw->_receiver );
			g_cLayerGate.Open();
			msg->SendReply( &cReply );
			STRACE(("\nServerApp %s: New Window %s (%.1f,%.1f,%.1f,%.1f)\n",
					fSignature.String(),title,frame.left,frame.top,frame.right,frame.bottom));

			break;
		}
		case AS_QUIT_WINDOW:
		{
			int32        hTopView;

			if ( msg->FindInt32( "top_view", &hTopView ) < 0 ) {
				printf( "Error: ServerApp::DispatchMessage() AR_CLOSE_WINDOW has no 'top_view' field\n" );
				break;
			}

			g_cLayerGate.Close();

			Layer* pcTopLayer = FindLayer( hTopView );
			if ( pcTopLayer == NULL ) {
				printf( "Error: ServerApp::DispatchMessage() attempt to close invalid window %ld\n", hTopView );
				g_cLayerGate.Open();
				break;
			}
			ServerWindow* pcWindow = pcTopLayer->GetWindow();
			if ( pcWindow == NULL ) {
				printf( "Error: ServerApp::DispatchMessage() top-view has no window! Can't close\n" );
				g_cLayerGate.Open();
				break;
			}
			m_cWindows.erase( pcWindow );
			delete pcWindow;
			g_cLayerGate.Open();
			break;
		}
		case AR_GET_FONT_FAMILY:
		{
			int32    nIndex = -1;
			font_family   zFamily;
			uint32 nFlags;

			msg->FindInt32( "index", &nIndex );

			zFamily[0] = '\0';
			int nError = fontserver->GetFamily( nIndex, zFamily, &nFlags );

			BMessage cReply;
			cReply.AddInt32( "error", nError );
			if ( nError >= 0 ) {
				cReply.AddString( "family", zFamily );
				cReply.AddInt32( "flags", nFlags );
			}

			msg->SendReply( &cReply );
			break;
		}
		case AR_GET_FONT_STYLE:
		{
			const char* pzFamily = "";
			int32                nIndex = -1;
			font_style    zStyle;
			uint32        nFlags;

			msg->FindString( "family", &pzFamily );
			msg->FindInt32( "index", &nIndex );

			zStyle[0] = '\0';
			int nError = fontserver->GetStyle( pzFamily, nIndex, zStyle, &nFlags );

			BMessage cReply;
			cReply.AddInt32( "error", nError );
			cReply.AddString( "style", zStyle );
			cReply.AddInt32( "flags", nFlags );

			msg->SendReply( &cReply );
			break;
		}
		case AR_CREATE_FONT:
		{
			FontNode* pcNode;
			try {
				pcNode = new FontNode;
			} catch(...) {
				printf( "ServerApp::DispatchMessage() Failed to create font node\n" );
				BMessage cReply;
				cReply.AddInt32( "handle", -ENOMEM );
				msg->SendReply( &cReply );
				break;
			}

			int hFontToken = int32(pcNode);
			if ( m_cFontLock.Lock() ) // NLS + FIXME: report an error if lock fails
			{
				m_cFonts.insert( pcNode );
				m_cFontLock.Unlock();
			}

			BMessage cReply;
			cReply.AddInt32( "handle", hFontToken );
			msg->SendReply( &cReply );
			break;
		}

		case AR_DELETE_FONT:
		{
			int32 hFont = -1;

			msg->FindInt32( "handle", &hFont );

			if ( m_cFontLock.Lock() )	// NLS
			{
				std::set<FontNode*>::iterator i = m_cFonts.find( (FontNode*)hFont );

				if ( i == m_cFonts.end() )
				{
					printf( "Error: Attempt to delete invalid font %ld\n", hFont );
					m_cFontLock.Unlock();
					break;
				}
				FontNode* pcNode = *i;
				m_cFonts.erase( pcNode );
				pcNode->Release();
				m_cFontLock.Unlock();
			}
			break;
		}
		case AR_SET_FONT_FAMILY_AND_STYLE:
		{
			int32 hFont = -1;
			const char* pzFamily = "";
			const char* pzStyle = "";

			msg->FindInt32( "handle", &hFont );
			msg->FindString( "family", &pzFamily );
			msg->FindString( "style", &pzStyle );

			BMessage cReply;

			if ( m_cFontLock.Lock() == false )	// NLS
			{
				printf( "Error: Failed to lock m_cFontLock while handling AR_SET_FONT_FAMILY_AND_STYLE request\n" );
				cReply.AddInt32( "error", -EINVAL );
				msg->SendReply( &cReply );
				break;
			}
			std::set<FontNode*>::iterator i = m_cFonts.find( (FontNode*)hFont );

			if ( i == m_cFonts.end() ) {
				printf( "Error : SetFontFamilyAndStyle() called on invalid font ID %ld\n", hFont );
				cReply.AddInt32( "error", -EINVAL );
				msg->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			FontNode* pcNode = *i;

			int nError = pcNode->SetFamilyAndStyle( pzFamily, pzStyle );

			if ( nError < 0 ) {
				cReply.AddInt32( "error", nError );
				msg->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			ServerFont* pcInstance = pcNode->GetInstance();

			if ( NULL == pcInstance ) {
				printf( "Error : SetFamilyAndStyle() FontNode::GetInstance() returned NULL\n" );
				cReply.AddInt32( "error", -ENOMEM );
				msg->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}

			cReply.AddInt32( "error", 0 );
			cReply.AddInt32( "ascender", pcInstance->GetAscender() );
			cReply.AddInt32( "descender", pcInstance->GetDescender() );
			cReply.AddInt32( "leading", pcInstance->GetLineGap() );

			msg->SendReply( &cReply );

			pcNode->NotifyDependent();
			m_cFontLock.Unlock();
			break;
		}
		case AR_SET_FONT_PROPERTIES:
		{
			int32   hFont     = -1;
			float vSize     = 8.0f;
			float vShear    = 0.0f;
			float vRotation = 0.0f;

			msg->FindInt32( "handle", &hFont );
			msg->FindFloat( "size", &vSize );
			msg->FindFloat( "rotation", &vRotation );
			msg->FindFloat( "shear", &vShear );

			BMessage cReply;

			if ( m_cFontLock.Lock() == false )	// NLS
			{
				printf( "Error: Failed to lock m_cFontLock while handling AR_SET_FONT_PROPERTIES request\n" );
				cReply.AddInt32( "error", -EINVAL );
				msg->SendReply( &cReply );
				break;
			}
			std::set<FontNode*>::iterator i = m_cFonts.find( (FontNode*)hFont );

			if ( i == m_cFonts.end() ) {
				printf( "Error : SetFontProperties() called on invalid font ID %ld\n", hFont );
				cReply.AddInt32( "error", -EINVAL );
				msg->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			FontNode* pcNode = *i;
			pcNode->SnapPointSize( &vSize );
			int nError = pcNode->SetProperties( int(vSize     * 64.0f),
												int(vShear    * 64.0f),
												int(vRotation * 64.0f) );
			if ( nError < 0 ) {
				cReply.AddInt32( "error", nError );
				msg->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}

			ServerFont* pcInstance = pcNode->GetInstance();

			if ( pcInstance == NULL ) {
				cReply.AddInt32( "error", -ENOMEM );
				msg->SendReply( &cReply );
				m_cFontLock.Unlock();
				break;
			}
			cReply.AddInt32( "error", 0 );
			cReply.AddInt32( "ascender", pcInstance->GetAscender() );
			cReply.AddInt32( "descender", pcInstance->GetDescender() );
			cReply.AddInt32( "leading", pcInstance->GetLineGap() );

			msg->SendReply( &cReply );
			pcNode->NotifyDependent();
			m_cFontLock.Unlock();
			break;
		}
		case AR_LOCK_DESKTOP:
		{
			int32 nDesktop;
			BMessage cReply;
			if ( msg->FindInt32( "desktop", &nDesktop ) != 0 )
			{
				cReply.AddInt32( "error", EINVAL );
			}
			else
			{
				display_mode sMode;
				std::string cBackdropPath;
				get_desktop_config( &nDesktop, &sMode, &cBackdropPath );

				cReply.AddInt32( "desktop", nDesktop );
				cReply.AddIPoint( "resolution", IPoint( sMode.virtual_width, sMode.virtual_height ) );
				cReply.AddInt32( "color_space", sMode.space );
				cReply.AddFloat( "refresh_rate", 60.0 );
				cReply.AddFloat( "h_pos", 0.0 );
				cReply.AddFloat( "v_pos", 0.0 );
				cReply.AddFloat( "h_size", sMode.virtual_width );
				cReply.AddFloat( "v_size", sMode.virtual_height );
				cReply.AddString( "backdrop_path", cBackdropPath.c_str() );
				cReply.AddInt32( "error", 0 );
			}
			msg->SendReply( &cReply );
			break;
		}
		case AR_UNLOCK_DESKTOP:
			break;
		case AR_GET_SCREENMODE_COUNT:
		{
			BMessage cReply;
			cReply.AddInt32( "count", 1);
			msg->SendReply( &cReply );
			break;
		}
		// Theoretically, we could just call the driver directly, but we will
		// call the CursorManager's version to allow for future expansion
		case AS_SHOW_CURSOR:
		{
			STRACE(("ServerApp %s: Show Cursor\n",fSignature.String()));
			cursormanager->ShowCursor();
			fCursorHidden=false;
			break;
		}
		case AS_HIDE_CURSOR:
		{
			STRACE(("ServerApp %s: Hide Cursor\n",fSignature.String()));
			cursormanager->HideCursor();
			fCursorHidden=true;
			break;
		}
		case AS_OBSCURE_CURSOR:
		{
			STRACE(("ServerApp %s: Obscure Cursor\n",fSignature.String()));
			cursormanager->ObscureCursor();
			break;
		}
		case AS_QUERY_CURSOR_HIDDEN:
		{
			STRACE(("ServerApp %s: Received IsCursorHidden request\n",fSignature.String()));
			BMessage cReply;
			cReply.AddBool( "hidden", fCursorHidden);
			msg->SendReply( &cReply );
			break;
		}
		case AS_SET_CURSOR_DATA:
		{
			// Attached data: 68 bytes of _appcursor data

			int8* cdata;
			ssize_t bufferLength;

			msg->FindData("cursor",B_RAW_TYPE,(const void **)&cdata, (ssize_t*)&bufferLength);

			// Because we don't want an overaccumulation of these particular
			// cursors, we will delete them if there is an existing one. It would
			// otherwise be easy to crash the server by calling SetCursor a
			// sufficient number of times
			if(fAppCursor)
				cursormanager->DeleteCursor(fAppCursor->ID());

			fAppCursor=new ServerCursor(cdata);
			fAppCursor->SetOwningTeam(fClientTeamID);
			fAppCursor->SetAppSignature(fSignature.String());
			cursormanager->AddCursor(fAppCursor);
			cursormanager->SetCursor(fAppCursor->ID());
			break;
		}
		case AS_SET_CURSOR_BCURSOR:
		{
			STRACE(("ServerApp %s: SetCursor via BCursor\n",fSignature.String()));
			// Attached data:
			// 1) bool flag to send a reply
			// 2) int32 token ID of the cursor to set
			// 3) port_id port to receive a reply. Only exists if the sync flag is true.
			bool sync;
			int32 ctoken;

			msg->FindBool( "sync", &sync );
			msg->FindInt32( "token", &ctoken );

			cursormanager->SetCursor(ctoken);

			if(sync)
			{
				// the application is expecting a reply, but plans to do literally nothing
				// with the data, so we'll just reuse the cursor token variable
				BMessage cReply;
				cReply.AddBool( "sync", sync);
				msg->SendReply( &cReply );
			}
			break;
		}
		case AS_CREATE_BCURSOR:
		{
			STRACE(("ServerApp %s: Create BCursor\n",fSignature.String()));
			// Attached data:
			// 1) 68 bytes of fAppCursor data

			int8* cdata;
			ssize_t bufferLength;

			msg->FindData("cursor",B_RAW_TYPE,(const void **)&cdata, (ssize_t*)&bufferLength);

			fAppCursor=new ServerCursor(cdata);
			fAppCursor->SetOwningTeam(fClientTeamID);
			fAppCursor->SetAppSignature(fSignature.String());
			cursormanager->AddCursor(fAppCursor);
			
			// Synchronous message - BApplication is waiting on the cursor's ID
			BMessage cReply;
			cReply.AddInt32( "id", fAppCursor->ID());
			msg->SendReply( &cReply );
			break;
		}
		case AS_DELETE_BCURSOR:
		{
			STRACE(("ServerApp %s: Delete BCursor\n",fSignature.String()));
			// Attached data:
			// 1) int32 token ID of the cursor to delete
			int32 ctoken;
			msg->FindInt32( "token", &ctoken );

			if(fAppCursor && fAppCursor->ID()==ctoken)
				fAppCursor=NULL;
			
			cursormanager->DeleteCursor(ctoken);
			break;
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ServerApp::_DispatchMessage( const void* pMsg, int nCode )
{
	switch( nCode )
	{
		case AS_CREATE_WINDOW:
		case AR_OPEN_BITMAP_WINDOW:
		case AS_QUIT_WINDOW:
		case AR_GET_FONT_FAMILY_COUNT:
		case AR_GET_FONT_FAMILY:
		case AR_GET_FONT_STYLE_COUNT:
		case AR_GET_FONT_STYLE:
		case AR_GET_FONT_SIZES:
		case AR_CREATE_FONT:
		case AR_DELETE_FONT:
		case AR_SET_FONT_FAMILY_AND_STYLE:
		case AR_SET_FONT_PROPERTIES:
		case AR_LOCK_DESKTOP:
		case AR_UNLOCK_DESKTOP:
		case AR_GET_SCREENMODE_COUNT:
		case AR_GET_SCREENMODE_INFO:
		case AR_SET_SCREEN_MODE:
		{
			try {
				BMessage cReq( pMsg, true );
				_DispatchMessage( &cReq );
			} catch(...) {
				printf( "Caught exception while handling request %d\n", nCode );
			}
			return( true );
		}
		case AS_DELETE_WINDOW:
		{
			g_cLayerGate.Close();
			ServerWindow* pcWindow = ((AR_DeleteWindow_s*)pMsg)->pcWindow;
			m_cWindows.erase( pcWindow );
			delete pcWindow;
			g_cLayerGate.Open();
			return( true );
		}
		case AR_GET_STRING_WIDTHS:
		{
			AR_GetStringWidths_s*      psReq = (AR_GetStringWidths_s*) pMsg;
			AR_GetStringWidthsReply_s* psReply = (AR_GetStringWidthsReply_s*) pMsg;

			int        nStrCount  = psReq->nStringCount;
			port_id        hReplyPort = psReq->hReply;

			if ( m_cFontLock.Lock() )	// NLS
			{
				std::set<FontNode*>::iterator i = m_cFonts.find( (FontNode*)psReq->hFontToken );

				if ( i == m_cFonts.end() ) {
					psReply->nError = -ENOENT;
					printf( "Error : GetStringWidths() called on invalid font ID %x\n", psReq->hFontToken );
					write_port( hReplyPort, 0, psReply, sizeof(AR_GetStringWidthsReply_s) );
					m_cFontLock.Unlock();
					return( true );
				}

				ServerFont* pcFont = (*i)->GetInstance();

				if ( NULL == pcFont ) {
					printf( "Error : GetStringWidths() FontNode::GetInstance() returned NULL\n" );

					psReply->nError = -ENOMEM;
					write_port( hReplyPort, 0, psReply, sizeof(AR_GetStringWidthsReply_s) );
					m_cFontLock.Unlock();
					return( true );
				}

				StringHeader_s*        psHdr = &psReq->sFirstHeader;

				for ( int i = 0 ; i < nStrCount ; ++i )
				{
					if ( psHdr->zString + psHdr->nLength >= reinterpret_cast<const char*>(pMsg) + 8192 ) {
						printf( "Error: GetStringWidths() invalid size %d\n", psHdr->nLength );
						psReply->nError = -EINVAL;
						break;
					}
					psReply->anLengths[i] = pcFont->GetStringWidth( psHdr->zString, psHdr->nLength );

					int nHdrSize = psHdr->nLength + sizeof( int );

					psHdr = (StringHeader_s*) (((int8*) psHdr) + nHdrSize);
				}
				psReply->nError = 0;
				write_port( hReplyPort, 0, psReply,
						sizeof(AR_GetStringWidthsReply_s) + (nStrCount - 1) * sizeof(int) );
				m_cFontLock.Unlock();
			}
			return( true );
		}
		case AR_GET_STRING_LENGTHS:
		{
			AR_GetStringLengths_s*      psReq   = (AR_GetStringLengths_s*) pMsg;
			AR_GetStringLengthsReply_s* psReply = (AR_GetStringLengthsReply_s*) pMsg;

			port_id          hReplyPort = psReq->hReply;
			int          nStrCount  = psReq->nStringCount;

			if ( m_cFontLock.Lock() )	// NLS
			{
				std::set<FontNode*>::iterator i = m_cFonts.find( (FontNode*)psReq->hFontToken );

				if ( i == m_cFonts.end() )
				{
					psReply->nError = -ENOENT;
					printf( "Error : GetStringLengths() called on invalid font ID %x\n", psReq->hFontToken );
					write_port( hReplyPort, 0, psReply, sizeof(AR_GetStringLengthsReply_s) );
					m_cFontLock.Unlock();
					return( true );
				}
				ServerFont* pcFont = (*i)->GetInstance();

				if ( NULL == pcFont )
				{
					printf( "Error : GetStringLengths() FontNode::GetInstance() returned NULL\n" );

					psReply->nError = -ENOMEM;
					write_port( hReplyPort, 0, psReply, sizeof(AR_GetStringLengthsReply_s) );
					m_cFontLock.Unlock();
					return( true );
				}

				StringHeader_s* psHdr = &psReq->sFirstHeader;

				for ( int i = 0 ; i < nStrCount ; ++i )
				{
					if ( psHdr->zString + psHdr->nLength >= reinterpret_cast<const char*>(pMsg) + 8192 )
					{
						psReply->nError = -EINVAL;
						printf( "Error: GetStringLengths() invalid size %d\n", psHdr->nLength );
						break;
					}

					psReply->anLengths[i] = pcFont->GetStringLength( psHdr->zString, psHdr->nLength,
																	psReq->nWidth, psReq->bIncludeLast );

					int nHdrSize = psHdr->nLength + sizeof( int );

					psHdr = (StringHeader_s*) (((int8*) psHdr) + nHdrSize);
				}
				psReply->nError = 0;
				write_port( hReplyPort, 0, psReply,
						sizeof(AR_GetStringLengthsReply_s) + (nStrCount - 1) * sizeof(int) );
				m_cFontLock.Unlock();
			}
			return( true );
		}
		case AS_CREATE_BITMAP:
		{
			STRACE(("ServerApp %s: Received BBitmap creation request\n",fSignature.String()));
			// Allocate a bitmap for an application
			
			// Attached Data: 
			// 1) BRect bounds
			// 2) color_space space
			// 3) int32 bitmap_flags
			// 4) int32 bytes_per_row
			// 5) int32 screen_id::id
			// 6) port_id reply port
			
			// Reply Code: SERVER_TRUE
			// Reply Data:
			//	1) int32 server token
			//	2) area_id id of the area in which the bitmap data resides
			//	3) int32 area pointer offset used to calculate fBasePtr
			
			AR_CreateBitmap_s*  psReq = (AR_CreateBitmap_s*) pMsg;
			g_cLayerGate.Close();
			
			// First, let's attempt to allocate the bitmap
			ServerBitmap *sbmp=bitmapmanager->CreateBitmap(psReq->rect,
															psReq->eColorSpc,
															psReq->flags,
															psReq->bytesperline,
															B_MAIN_SCREEN_ID);

			STRACE(("ServerApp %s: Create Bitmap (%.1f,%.1f,%.1f,%.1f)\n",
						fSignature.String(),r.left,r.top,r.right,r.bottom));

			if(sbmp)
			{
				fBitmapList->AddItem(sbmp);
				
				AR_CreateBitmapReply_s sReply(sbmp->Token(), sbmp->Area(), sbmp->AreaOffset());
				write_port(psReq->hReply, 0, &sReply, sizeof(sReply ));
			}
			else
			{
				// alternatively, if something went wrong, we reply with ENOMEM
				AR_CreateBitmapReply_s sReply( -ENOMEM, -1, 0 );
				write_port(psReq->hReply, 0, &sReply, sizeof(sReply));
			}
			
			g_cLayerGate.Open();
			return( true );
		}
		case AS_DELETE_BITMAP:
		{
			STRACE(("ServerApp %s: received BBitmap delete request\n",fSignature.String()));
			// Delete a bitmap's allocated memory

			// Attached Data:
			// 1) int32 token
		
			AR_DeleteBitmap_s*  psReq = (AR_DeleteBitmap_s*) pMsg;
			g_cLayerGate.Close();
			
			ServerBitmap *sbmp=FindBitmap(psReq->m_nHandle);
			if(sbmp)
			{
				STRACE(("ServerApp %s: Deleting Bitmap %ld\n",fSignature.String(),bmp_id));

				fBitmapList->RemoveItem(sbmp);
				bitmapmanager->DeleteBitmap(sbmp);
			}
			g_cLayerGate.Open();
			return( true );
		}
		case AR_CREATE_SPRITE:
		{
			AR_CreateSprite_s* psReq = (AR_CreateSprite_s*) pMsg;
			ServerBitmap*  pcBitmap = NULL;
			int    nError;
			uint32 nHandle;

			if ( psReq->m_nBitmap != -1 )
			{
				pcBitmap=FindBitmap(psReq->m_nBitmap);

				if ( pcBitmap == NULL )
				{
					printf( "Error: Attempt to create sprite with invalid bitmap %d\n", psReq->m_nBitmap );
				}
			}
			g_cLayerGate.Close();
			SrvSprite* pcSprite = new SrvSprite( static_cast<IRect>(psReq->m_cFrame), IPoint( 0, 0 ), IPoint( 0, 0 ),
												g_pcTopView->GetBitmap(),
												pcBitmap );
			g_cLayerGate.Open();

			m_cSprites.insert( pcSprite );
			nHandle = (uint32) pcSprite;
			nError  = 0;


			AR_CreateSpriteReply_s sReply( nHandle, nError );
			write_port( psReq->m_hReply, 0, &sReply, sizeof( sReply ) );
			return( true );
		}
		case AR_DELETE_SPRITE:
		{
			AR_DeleteSprite_s* psReq = (AR_DeleteSprite_s*) pMsg;
			SrvSprite* pcSprite = (SrvSprite*) psReq->m_nSprite;


			std::set<SrvSprite*>::iterator i = m_cSprites.find(pcSprite);
			if ( i != m_cSprites.end() )
			{
				m_cSprites.erase(i);

				g_cLayerGate.Close();
				delete pcSprite;
				g_cLayerGate.Open();

			}
			else
			{
				printf( "Error: Attempt to delete invalid sprite %ld\n", psReq->m_nSprite );
			}
			return( true );
		}
		case AR_MOVE_SPRITE:
		{
			AR_MoveSprite_s* psReq = (AR_MoveSprite_s*) pMsg;
			SrvSprite* pcSprite = (SrvSprite*) psReq->m_nSprite;

			if ( m_cSprites.find(pcSprite) != m_cSprites.end() )
			{
				g_cLayerGate.Close();
//        g_pcDispDrv->MouseOff();
				pcSprite->MoveTo( IPoint(psReq->m_cNewPos) );
//        g_pcDispDrv->MouseOn();
				g_cLayerGate.Open();
			}
			return( true );
		}
		case B_QUIT_REQUESTED:
		{
			std::set<ServerWindow*>::iterator i;

			for ( i = m_cWindows.begin() ; i != m_cWindows.end() ; ++i )
			{
				ServerWindow* pcWnd = *i;
				pcWnd->Quit();
			}
			return( false );
		}
		case AS_LAYER_CREATE:
		case AS_LAYER_DELETE:
		case WR_TOGGLE_VIEW_DEPTH:
		{
			BMessage cReq( pMsg, true );

			int32 hTopView;

			cReq.FindInt32( "top_view", &hTopView );

			g_cLayerGate.Lock();

			Layer* pcTopLayer = FindLayer( hTopView );
			if ( pcTopLayer == NULL ) {
				printf( "Error: ServerApp::DispatchMessage() message to invalid window %ld\n", hTopView );
				g_cLayerGate.Unlock();
				return( true );
			}
			ServerWindow* pcWindow = pcTopLayer->GetWindow();
			if ( pcWindow == NULL ) {
				printf( "Error: ServerApp::DispatchMessage() top-view has no window! Can't forward message\n" );
				g_cLayerGate.Unlock();
				return( true );
			}
			g_cLayerGate.Unlock();
			pcWindow->DispatchMessage( &cReq );
			return( true );
		}
		break;
		case WR_GET_VIEW_FRAME:
		case WR_RENDER:
		case WR_GET_PEN_POSITION:
		{
			WR_Request_s* psReq = static_cast<WR_Request_s*>( (void*)pMsg );

			g_cLayerGate.Lock();

			Layer* pcTopLayer = FindLayer( psReq->m_hTopView );
			if ( pcTopLayer == NULL )
			{
				printf( "Error: ServerApp::DispatchMessage() message to invalid window %d\n", psReq->m_hTopView );
				g_cLayerGate.Unlock();
				return( true );
			}
			ServerWindow* pcWindow = pcTopLayer->GetWindow();
			if ( pcWindow == NULL )
			{
				printf( "Error: ServerApp::DispatchMessage() top-view has no window! Can't forward message\n" );
				g_cLayerGate.Unlock();
				return( true );
			}
			g_cLayerGate.Unlock();
			pcWindow->DispatchMessage( pMsg, nCode );
			return( true );
		}
		default:
		{
			printf( "Error: ServerApp::DispatchMessage() unknown message %d\n", nCode );
			return( true );
		}
	}
}

/*!
	\brief Looks up a ServerApp's ServerBitmap in its list
	\param token ID token of the bitmap to find
	\return The bitmap having that ID or NULL if not found
*/
ServerBitmap *ServerApp::FindBitmap(int32 token)
{
	ServerBitmap *temp;
	for(int32 i=0; i<fBitmapList->CountItems();i++)
	{
		temp=(ServerBitmap*)fBitmapList->ItemAt(i);
		if(temp && temp->Token()==token)
			return temp;
	}
	return NULL;
}

team_id ServerApp::ClientTeamID()
{
	return fClientTeamID;
}

