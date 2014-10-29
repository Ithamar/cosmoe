#ifndef	_OPENBEOS_APP_SERVER_H_
#define	_OPENBEOS_APP_SERVER_H_

#include <Locker.h>
#include <List.h>
#include <Region.h>
#include <GraphicsDefs.h>
#include <Bitmap.h>
#include <util/array.h>
#include <MessageQueue.h>
#include <SupportDefs.h>
#include <Decorator.h>

#include "fontnode.h"

typedef        struct TextFont        TextFont_s;

class ServerWindow;
class ServerBitmap;

class Layer;
class BMessage;
class ServerApp;
class DisplayDriver;
class BPortLink;
class CursorManager;
class BitmapManager;

extern Layer*            g_pcTopView;
extern Array<Layer>*     g_pcLayers;

void config_changed();

/*!
	\class AppServer AppServer.h
	\brief main manager object for the app_server
	
	File for the main app_server thread. This particular thread monitors for
	application start and quit messages. It also starts the housekeeping threads
	and initializes most of the server's globals.
*/
class AppServer
{
public:
	AppServer(void);
	~AppServer(void);

	static	int32 PollerThread(void *data);
	static	int32 PicassoThread(void *data);
	thread_id Run(void);
	void MainLoop(void);

	FontNode*				GetWindowTitleFont() const { return( m_pcWindowTitleFont ); }
	FontNode*				GetToolWindowTitleFont() const { return( m_pcToolWindowTitleFont ); }

	void					R_ClientDied( thread_id hThread );

	int					LoadWindowDecorator( const std::string& cPath );

	port_id					m_hWndReplyPort;
	
	bool LoadDecorator(const char *path);
	void InitDecorators(void);
	void DispatchMessage(BMessage *msg);
	void Broadcast(int32 code);

	ServerApp* FindApp(const char *sig);
	
private:
	friend	Decorator*	new_decorator(Layer* pcView, int32 wlook, int32 wfeel,
			uint32 wflags);
	friend void SDLEventTranslator(void *arg);
	// global function pointer
	create_decorator	*make_decorator;
	
	port_id	fMessagePort,
			fMousePort;

	image_id fDecoratorID;
	
	BString fDecoratorName;

	bool fQuittingServer,
		fExitPoller;

	BList *fAppList;
	thread_id fPollerThreadID,
			  fPicassoThreadID;

	sem_id 	fActiveAppLock,
			fAppListLock,
			fDecoratorLock;
	
	FontNode*				m_pcWindowTitleFont;
	FontNode*				m_pcToolWindowTitleFont;

	DisplayDriver *fDriver;
};

extern CursorManager *cursormanager;
extern BitmapManager *bitmapmanager;
extern AppServer *app_server;

#endif
