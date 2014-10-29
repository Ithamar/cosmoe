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
//	File Name:		ServerApp.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Server-side BApplication counterpart
//  
//------------------------------------------------------------------------------
#ifndef _SERVERAPP_H_
#define _SERVERAPP_H_

#include <Locker.h>

#include <set>
#include <string>
#include <vector>



class ServerWindow;
class FontNode;
class SrvSprite;
class SFontInstance;
class BMessenger;

class ServerCursor;

/*!
	\class ServerApp ServerApp.h
	\brief Counterpart to BApplication within the app_server
*/
class ServerApp
{
public:
	ServerApp(port_id hEventPort,
		team_id clientTeamID, int32 handlerID, const char *signature);
	virtual						~ServerApp(void);
							
	bool Run(void);
	static int32 MonitorApp(void *data);	
	void Lock(void);
	void Unlock(void);
	bool IsLocked(void);
	
	static ServerApp*	FindApp( team_id hProc );
	static void				FindApps( BMessage& ioMessage, const char* inMimeSig );

	FontNode*				GetFont( uint32 nID );
	static void				ReplaceDecorators();
	static void				NotifyColorCfgChanged();
	
	/*!
		\brief Determines whether the application is the active one
		\return true if active, false if not.
	*/
	bool IsActive(void) const { return fIsActive; }
	
	void Activate(bool value);
	bool PingTarget(void);

	void SendMessageToClient( BMessage* msg ) const;

	void SetAppCursor(void);
	ServerBitmap *FindBitmap(int32 token);

	team_id	ClientTeamID();

protected:
	friend class AppServer;
	friend class ServerWindow;

	static ServerApp*	s_pcFirstApp;
	static BLocker			s_cAppListLock;

	void _DispatchMessage(BMessage *msg);
	bool _DispatchMessage(const void* msg, int nCode);

	ServerApp*			m_pcNext;

	port_id _sender,_receiver;
	BString fSignature;
	thread_id fMonitorThreadID;

	team_id fClientTeamID;
	
	BList *fSWindowList,
		  *fBitmapList,
		  *fPictureList;
	ServerCursor *fAppCursor;
	sem_id fLockSem;
	bool fCursorHidden;
	bool fIsActive;
	int32 fHandlerToken;
	BLocker					m_cFontLock;
	BMessenger*				m_pcAppTarget;

	std::set<ServerWindow*>	m_cWindows;
	std::set<FontNode*>		m_cFonts;
	std::set<SrvSprite*>	m_cSprites;
};

#endif
