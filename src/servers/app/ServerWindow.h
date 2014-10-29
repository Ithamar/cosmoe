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
//	File Name:		ServerWindow.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BWindow class
//  
//------------------------------------------------------------------------------
#ifndef _SERVERWIN_H_
#define _SERVERWIN_H_

#include "Layer.h"

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <OS.h>
#include <Locker.h>
#include <Rect.h>
#include <String.h>
#include <Window.h>
#include <Message.h>

#include <string>

class BString;
class BMessenger;
class BPoint;
class BMessage;
class ServerApp;
class Decorator;
class BPortLink;
class WinBorder;
class ServerBitmap;
class SrvSprite;
struct WR_Render_s;

#define AS_UPDATE_DECORATOR 'asud'
#define AS_UPDATE_COLORS 'asuc'
#define AS_UPDATE_FONTS 'asuf'

/*!
	\class ServerWindow ServerWindow.h
	\brief Shadow BWindow class
	
	A ServerWindow handles all the intraserver tasks required of it by its BWindow. There are 
	too many tasks to list as being done by them, but they include handling View transactions, 
	coordinating and linking a window's WinBorder half with its messaging half, dispatching 
	mouse and key events from the server to its window, and other such things.
*/
class ServerWindow
{
public:
	ServerWindow(BRect rect, const char *string, uint32 wlook, uint32 wfeel,
			void*			pTopView,
		uint32 wflags, ServerApp *winapp, port_id winport,
		uint32			nDesktopMask );

	ServerWindow(ServerApp* pcApp, ServerBitmap* pcBitmap);
					
	virtual ~ServerWindow(void);
	
	void Init();
	
	// ServerWindow must be locked for these ones.	
	void ReplaceDecorator(void);
	void Quit(void);
	void Show(void);
	void Hide(void);
	void SetFocus(bool value);
	bool HasFocus(void);

	void			NotifyWindowFontChanged( bool bToolWindow );
	void			BeginRegionUpdate();

	BMessenger*	GetAppTarget( void ) const  { return( _appTarget ); }

	void			SetBitmap( ServerBitmap* pcBitmap ) { fTopLayer->SetBitmap( pcBitmap ); }

	// methods for sending various messages to client.
	void WorkspaceActivated(int32 workspace, const IPoint cNewRes, color_space cspace);
	void WindowActivated(bool active);
	void ScreenModeChanged(const IPoint cNewRes, const color_space cspace);

	void SetFrame(const BRect &rect);
	BRect Frame(bool bClient = true);

	Layer*			GetTopView() const;
	Layer*			GetClientView() const { return(fTopLayer); }

	status_t Lock(void);
	void Unlock(void);
	bool IsLocked(void) const;
	
	//! Returns the index of the workspaces to which it belongs
	int32 GetWorkspaceIndex(void) { return fWorkspaces; }
	void			SetDesktopMask( uint32 nMask );

	// util methods.	
	void SendMessageToClient( BMessage *msg ) const;

	// a few, not that important methods returning some internal settings.	
	int32 Look(void) const { return fLook; }
	int32 Feel(void) const { return fFeel; }
	uint32 Flags(void) const { return fFlags; }
	uint32 Workspaces(void) const { return fWorkspaces; }

	// to who we belong. who do we own. our title.
	ServerApp *App(void) const { return fServerApp; }
	WinBorder *GetWinBorder(void) const { return fWinBorder; }
	const char *Title(void) const { return fTitle.String(); }

	// related thread/team_id(s).
	team_id ClientTeamID(void) const { return fClientTeamID; }
	thread_id ThreadID(void) const { return fMonitorThreadID;}

	// server "private" - try not to use.
	void QuietlySetWorkspaces(uint32 wks) { fWorkspaces = wks; }
	void QuietlySetFeel(int32 feel) { fFeel = feel; }
	int32 ClientToken(void) const { return fHandlerToken; }
	
	port_id			GetMsgPort(void)        { return( _receiver ); }

	bool			IsOffScreen() const        { return( m_bOffscreen ); }

	bool			HasPendingSizeEvents( Layer* pcLayer );

	static void		SendKbdEvent( int nKeyCode, uint32 nQual, const char* pzConvString, const char* zRawString );
	static void		HandleInputEvent( BMessage* pcEvent );
	static void		HandleMouseTransaction();

	struct DeskTopState_s
	{
		DeskTopState_s() { m_pcNextWindow = NULL; }
		BRect   m_cFrame;
		ServerWindow* m_pcNextWindow;
		ServerWindow* m_pcNextSystemWindow;
	} m_asDTState[32];

private:
	// message handle methods.
	bool DispatchMessage( BMessage* pcReq );
	bool DispatchMessage( const void* psMsg, int nCode );
	void DispatchGraphicsMessage( WR_Render_s* psPkt );
	static int32 MonitorWin(void *data);


protected:	
	friend class ServerApp;
	friend class WinBorder;
	friend class Screen; 
	friend class Layer;
	
	void			MouseMoved( BMessage* pcEvent, int nTransit );
	void			MouseDown( BMessage* pcEvent );
	void			MouseUp( BMessage* pcEvent, bool bSendDragMsg );

	static void		HandleMouseDown( const BPoint& cMousePos, int nButton, BMessage* pcEvent );
	static void		HandleMouseMoved( const BPoint& cMousePos, BMessage* pcEvent );
	static void		HandleMouseUp( const BPoint& cMousePos, int nButton, BMessage* pcEvent );

	ServerBitmap*		GetBitmap() const { return( fTopLayer->GetBitmap() ); }

	static ServerWindow*    s_pcLastMouseWindow;
	static ServerWindow*    s_pcDragWindow;
	static SrvSprite*    s_pcDragSprite;
	static BMessage  s_cDragMessage;
	static bool          s_bIsDragging;

	ServerWindow*		m_pcNextGlobal;

	BString fTitle;
	int32 fLook;
	int32 fFeel;
	int32 fFlags;
	uint32 fWorkspaces;
	bool fIsActive;

	ServerApp *fServerApp;
	WinBorder *fWinBorder;
	
	team_id fClientTeamID;
	thread_id fMonitorThreadID;

	port_id	_receiver;	// Messages from window
	port_id fClientWinPort;
	port_id	winLooperPort;
	
	BLocker fLocker;
	BRect fFrame;
	uint32 fToken;
	int32 fHandlerToken;
	
	Layer *fTopLayer;

	ServerBitmap*		m_pcUserBitmap;
	bigtime_t		m_nLastHitTime;      // Time of last mouse click
	bool			m_bBorderHit;
	bool			m_bOffscreen;        // True for bitmap windows
	bool			m_bNeedRegionUpdate;
	Decorator* m_pcDecorator;
	BMessenger*	_appTarget;
};

#endif
