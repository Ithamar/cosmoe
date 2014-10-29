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
//	File Name:		AppServer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	main manager object for the app_server
//  
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdexcept>
#include <errno.h>

#include "../config.h"
#include <SupportDefs.h>
#include <OS.h>
#include <kernel.h>
#include <image.h>

#include <InterfaceDefs.h>
#include <Locker.h>
#include <String.h>
#include <PortLink.h>
#include <RegistrarDefs.h>
#include "AppServer.h"
#include "CursorHandler.h"
#include "DisplayDriver.h"
#include "ServerApp.h"
#include "ServerCursor.h"
#include <ServerProtocol.h>
#include "ServerWindow.h"
#include "DefaultDecorator.h"
#include "RGBColor.h"
#include "BitmapManager.h"
#include "CursorManager.h"
#include "Layer.h"
#include "ServerBitmap.h"
#include "clipboard.h"
#include "config.h"
#include "sprite.h"
#include "FontServer.h"
#include "Desktop.h"
#include "BitmapDriver.h"
#include "ServerConfig.h"

#include <macros.h>

void  ScreenShot();

//#define DEBUG_KEYHANDLING
//#define DEBUG_SERVER

#ifdef DEBUG_KEYHANDLING
#	include <stdio.h>
#	define KBTRACE(x) printf x
#else
#	define KBTRACE(x) ;
#endif

#ifdef DEBUG_SERVER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif
// Globals

Desktop *desktop;

//! Used to access the app_server from new_decorator
AppServer *app_server=NULL;

//! Default background color for workspaces
RGBColor workspace_default_color(51,102,160);

//! System-wide GUI color object
ColorSet gui_colorset;

Array<Layer>*      g_pcLayers;

Layer*          g_pcTopView = NULL;

/*!
	\brief Constructor
	
	This loads the default fonts, allocates all the major global variables, spawns the main housekeeping
	threads, loads user preferences for the UI and decorator, and allocates various locks.
*/
AppServer::AppServer(void)
{
	fMousePort= create_port(200,SERVER_INPUT_PORT);
	fMessagePort= create_port(200,SERVER_PORT_NAME);

	fAppList= new BList(0);
	fQuittingServer= false;
	fExitPoller= false;
	make_decorator= NULL;

	// We need this in order for new_decorator to be able to instantiate new decorators
	app_server=this;

	// Create the font server and scan the proper directories.
	fontserver=new FontServer;
	fontserver->Lock();

	// Used for testing purposes

	// TODO: Re-enable scanning of all font directories when server is actually put to use
	fontserver->ScanDirectory("/usr/share/fonts/ttf/cosmoe");

	fontserver->Unlock();

	printf( "Load default fonts\n" );

	m_pcWindowTitleFont = new FontNode;
	m_pcToolWindowTitleFont = new FontNode;

	const font_properties* psProp;

	psProp = AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_WINDOW );
	m_pcWindowTitleFont->SetProperties( *psProp );

	psProp = AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_TOOL_WINDOW );
	m_pcToolWindowTitleFont->SetProperties( *psProp );

	// Load the GUI colors here and set the global set to the values contained therein. If this
	// is not possible, set colors to the defaults
	if(!LoadGUIColors(&gui_colorset))
		gui_colorset.SetToDefaults();

	InitDecorators();

	// Set up the Desktop
	desktop= new Desktop();
	desktop->Init();

	// Create the cursor manager. Object declared in CursorManager.cpp
	cursormanager= new CursorManager();
	cursormanager->SetCursor(B_CURSOR_DEFAULT);
	
	// Create the bitmap allocator. Object declared in BitmapManager.cpp
	bitmapmanager= new BitmapManager();

	// This is necessary to mediate access between the Poller and app_server threads
	fActiveAppLock= create_sem(1,"app_server_active_sem");

	// This locker is for app_server and Picasso to vie for control of the ServerApp list
	fAppListLock= create_sem(1,"app_server_applist_sem");

	// This locker is to mediate access to the make_decorator pointer
	fDecoratorLock= create_sem(1,"app_server_decor_sem");
	
	// Spawn our input-polling thread
	fPollerThreadID= spawn_thread(PollerThread, "Poller", B_NORMAL_PRIORITY, this);
	if (fPollerThreadID >= 0)
		resume_thread(fPollerThreadID);

	// Spawn our thread-monitoring thread
	fPicassoThreadID= spawn_thread(PicassoThread,"Picasso", B_NORMAL_PRIORITY, this);
	if (fPicassoThreadID >= 0)
		resume_thread(fPicassoThreadID);

	fDecoratorName="Default";
}

/*!
	\brief Destructor
	
	Reached only when the server is asked to shut down in Test mode. Kills all apps, shuts down the 
	desktop, kills the housekeeping threads, etc.
*/
AppServer::~AppServer(void)
{

	ServerApp *tempapp;
	int32 i;
	acquire_sem(fAppListLock);
	for(i=0;i<fAppList->CountItems();i++)
	{
		tempapp=(ServerApp *)fAppList->ItemAt(i);
		if(tempapp!=NULL)
			delete tempapp;
	}
	delete fAppList;
	release_sem(fAppListLock);

	delete bitmapmanager;
	delete cursormanager;

	delete desktop;

	// If these threads are still running, kill them - after this, if exit_poller
	// is deleted, who knows what will happen... These things will just return an
	// error and fail if the threads have already exited.
	kill_thread(fPollerThreadID);
	kill_thread(fPicassoThreadID);

	delete fontserver;
	
	make_decorator=NULL;
}

/*!
	\brief Thread function for polling and handling input messages
	\param data Pointer to the app_server to which the thread belongs
	\return Throwaway value - always 0
*/
int32 AppServer::PollerThread(void *data)
{
	// This thread handles nothing but input messages for mouse and keyboard
	AppServer *appserver=(AppServer*)data;
	BPortLink mousequeue(-1,appserver->fMousePort);
	int32 code=0;
	status_t err=B_OK;

	for(;;)
	{
		STRACE(("info: AppServer::PollerThread listening on port %ld.\n", appserver->fMousePort));
		err=mousequeue.GetNextReply(&code);
		
		if(err<B_OK)
		{
			STRACE(("PollerThread:mousequeue.GetNextReply failed\n"));
			continue;
		}
		
		switch(code)
		{
			// We don't need to do anything with these two, so just pass them
			// onto the active application. Eventually, we will end up passing 
			// them onto the window which is currently under the cursor.
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_WHEEL_CHANGED:
			case B_MOUSE_MOVED:
				desktop->MouseEventHandler(code,mousequeue);
				break;

			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
				desktop->KeyboardEventHandler(code,mousequeue);
				break;

			default:
				STRACE(("AppServer::Poller received unexpected code %lx\n",code));
				break;
		}
		
		if(appserver->fExitPoller)
			break;
	}
	return err;
}

/*!
	\brief Thread function for watching for dead apps
	\param data Pointer to the app_server to which the thread belongs
	\return Throwaway value - always 0
*/
int32 AppServer::PicassoThread(void *data)
{
	AppServer	*appserver=(AppServer*)data;
	for(;;)
	{
		
		// if poller thread has to exit, so do we - I just was too lazy
		// to rename the variable name. ;)
		if(appserver->fExitPoller)
			break;

		// we do this every other second so as not to suck *too* many CPU cycles
		snooze(2000000);
	}
	return 0;
}

/*!
	\brief The call that starts it all...
	\return Always 0
*/
thread_id AppServer::Run(void)
{
	MainLoop();
	printf("Returned from MainLoop!\n");
	return 0;
}

//! Main message-monitoring loop for the regular message port - no input messages!
void AppServer::MainLoop(void)
{
	int32 code=0;

	enum        { e_MessageSize = 1024*64 };
	char* pBuffer = new char[ e_MessageSize ];
	BMessage* cReq = NULL;

	set_thread_priority( find_thread( NULL ), B_DISPLAY_PRIORITY );

	while(1)
	{
		STRACE(("info: AppServer::MainLoop listening on port %ld.\n", fMessagePort));

		if ( read_port_etc(fMessagePort, &code, pBuffer, e_MessageSize, 0UL, 1000000 ) >= 0 )
		{
			if ( AppserverConfig::GetInstance()->IsDirty() )
			{
				static bigtime_t nLastSaved = 0;
				bigtime_t nCurTime = system_time();
				if ( nCurTime > nLastSaved + 1000000 )
				{
					AppserverConfig::GetInstance()->SaveConfig();
					nLastSaved = nCurTime;
				}
			}
			
			if (code == 'pjpp')
			{
				cReq = new BMessage(pBuffer, true);
				code = cReq->what;
			}
			
			if (code == B_QUIT_REQUESTED)
			{
				cReq = new BMessage(B_QUIT_REQUESTED);
			}

			switch(code)
			{
				case -1:
					R_ClientDied( ((DR_ThreadDied_s*)pBuffer)->hThread );
					break;

				case B_QUIT_REQUESTED:
				case AS_CREATE_APP:
				case AS_DELETE_APP:
				case AS_GET_SCREEN_MODE:
				case AS_UPDATED_CLIENT_FONTLIST:
				case AS_QUERY_FONTS_CHANGED:
				case AS_SET_UI_COLORS:
				case AS_GET_UI_COLOR:
				case AS_SET_DECORATOR:
				case AS_GET_DECORATOR:
				case AS_R5_SET_DECORATOR:
				case DR_GET_DEFAULT_FONT:
				{
					if (cReq)
					{
						DispatchMessage(cReq);
						delete cReq;
						cReq = NULL;
					}
					else
						printf("SA:Dispatch cReq was NULL!");
					break;
				}

				default:
					printf("WARNING : AppServer::Run() Unknown command %ld\n", code);
					printf("          Unknown command (%c%c%c%c)\n", int(code & 0xFF000000) >> 24,
						int(code & 0xFF0000) >> 16, int(code & 0xFF00) >> 8, (int)code & 0xFF);
					pBuffer[80] = '\0';
					printf("          Raw buffer contains: \"%s\"\n", pBuffer);
					break;
			}
		}
		
		if(code==AS_DELETE_APP || (code==B_QUIT_REQUESTED && DISPLAYDRIVER!=HWDRIVER))
		{
			if(fQuittingServer== true && fAppList->CountItems()== 0)
				break;
		}
	}
	
	printf( "AppServer::Run() has completed" );
}


/*!
	\brief Loads the specified decorator and sets the system's decorator to it.
	\param path Path to the decorator to load
	\return True if successful, false if not.

	If the server cannot load the specified decorator, nothing changes. Passing a 
	NULL string to this function sets the decorator	to the internal one.
*/
bool AppServer::LoadDecorator(const char *path)
{
	// Loads a window decorator based on the supplied path and forces a decorator update.
	// If it cannot load the specified decorator, it will retain the current one and
	// return false. Note that passing a NULL string to this function sets the decorator
	// to the internal one.

	// passing the string "Default" will set the window decorator to the app_server's
	// internal one

	create_decorator	*pcreatefunc= NULL;
	status_t			stat;
	image_id			addon;
	
	addon= load_add_on(path);
	if(addon < 0)
		return false;

	// As of now, we do nothing with decorator versions, but the possibility exists
	// that the API will change even though I cannot forsee any reason to do so. If
	// we *did* do anything with decorator versions, the assignment to a global would
	// go here.

	// Get the instantiation function
	stat= get_image_symbol(addon, "instantiate_decorator", -1, (void**)&pcreatefunc);
	if(stat != B_OK)
	{
		STRACE(( "Error: window decorator '%s' does not export instantiate_decorator()\n", path ));
		unload_add_on(addon);
		return false;
	}

	make_decorator=pcreatefunc;
	fDecoratorID=addon;
	return true;
}

//! Loads decorator settings on disk or the default if settings are invalid
void AppServer::InitDecorators(void)
{
	LoadDecorator( AppserverConfig::GetInstance()->GetWindowDecoratorPath().c_str());
}

/*!
	\brief Message handling function for all messages sent to the app_server
	\param code ID of the message sent
	\param buffer Attachment buffer for the message.
	
*/
void AppServer::DispatchMessage(BMessage *msg)
{
	switch(msg->what)
	{
		case AS_CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication

			// Attached data:
			// 1) port_id - receiver port of a regular app
			// 2) team_id - app's team ID
			// 3) int32 - handler token of the regular app
			// 4) char * - signature of the regular app

			// Find the necessary data
			team_id	clientTeamID=-1;
			int32 htoken=B_NULL_TOKEN;
			port_id app_port=-1;
			const char *app_signature;

			msg->FindInt32( "process_id", &clientTeamID );
			msg->FindInt32( "event_port", &app_port);
			msg->FindString( "app_name", &app_signature);

			// Create the ServerApp subthread for this app
			acquire_sem(fAppListLock);

			ServerApp *newapp=NULL;
			newapp= new ServerApp(app_port, clientTeamID,
					htoken, app_signature);

			// add the new ServerApp to the known list of ServerApps
			fAppList->AddItem(newapp);
			
			release_sem(fAppListLock);

			BMessage cReply;
			cReply.AddInt32( "app_cmd_port", newapp->_receiver );

			for ( int i = 0 ; i < COL_COUNT ; ++i )
			{
				cReply.AddInt32("cfg_colors",
								_get_uint32_color(get_default_color(static_cast<default_color_t>(i))) );
			}
			msg->SendReply( &cReply );

			break;
		}
		case AS_DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.
			
			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted
			
			int32 	i=0,
					appnum=fAppList->CountItems();
			
			ServerApp *srvapp=NULL;
			thread_id srvapp_id=-1;

			msg->FindInt32( "thread", &srvapp_id );

			acquire_sem(fAppListLock);

			// Run through the list of apps and nuke the proper one
			for(i= 0; i < appnum; i++)
			{
				srvapp=(ServerApp *)fAppList->ItemAt(i);

				if(srvapp != NULL && srvapp->fMonitorThreadID== srvapp_id)
				{
					srvapp=(ServerApp *)fAppList->RemoveItem(i);
					if(srvapp)
					{
						status_t		temp;
						wait_for_thread(srvapp_id, &temp);
						delete srvapp;
						srvapp= NULL;
					}
					break;	// jump out of our for() loop
				}
			}

			release_sem(fAppListLock);
			break;
		}
		case AS_UPDATED_CLIENT_FONTLIST:
		{
			// received when the client-side global font list has been
			// refreshed
			fontserver->Lock();
			fontserver->FontsUpdated();
			fontserver->Unlock();
			break;
		}
		case DR_GET_DEFAULT_FONT:
		{
			const char* pzConfigName;
			BMessage cReply;

			if ( msg->FindString( "config_name", &pzConfigName ) != 0 ) {
				cReply.AddInt32( "error", -EINVAL );
				msg->SendReply( &cReply );
				break;
			}
			const font_properties* psProps = AppserverConfig::GetInstance()->GetFontConfig( pzConfigName );
			if ( psProps == NULL ) {
				cReply.AddInt32( "error", -EINVAL );
				msg->SendReply( &cReply );
				break;
			}
			cReply.AddString( "family", psProps->m_cFamily.c_str() );
			cReply.AddString( "style", psProps->m_cStyle.c_str() );
			cReply.AddFloat( "size", psProps->m_vSize );
			cReply.AddFloat( "shear", psProps->m_vShear );
			cReply.AddFloat( "rotation", psProps->m_vRotation );
			cReply.AddInt32( "error", 0 );
			msg->SendReply( &cReply );
			break;
		}
		case B_QUIT_REQUESTED:
		{
			// Attached Data:
			// none
			
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the server
			// is compiled as a regular Be application.
			if(DISPLAYDRIVER== HWDRIVER)
				break;
			
			Broadcast(B_QUIT_REQUESTED);

			// we have to wait until *all* threads have finished!
			ServerApp	*app= NULL;
			acquire_sem(fAppListLock);
			thread_info tinfo;
			
			for(int32 i= 0; i < fAppList->CountItems(); i++)
			{
				app=(ServerApp*)fAppList->ItemAt(i);
				if(!app)
					continue;

				// Instead of calling wait_for_thread, we will wait a bit, check for the
				// thread_id. We will only wait so long, because then the app is probably crashed
				// or hung. Seeing that being the case, we'll kill its BApp team and fake the
				// quit message
				if(get_thread_info(app->fMonitorThreadID, &tinfo)==B_OK)
				{
					bool killteam=true;
					
					for(int32 j=0; j<5; j++)
					{
						snooze(1000);	// wait half a second for it to quit
						if(get_thread_info(app->fMonitorThreadID, &tinfo)!=B_OK)
						{
							killteam=false;
							break;
						}
					}
					
					if(killteam)
					{
						kill_team(app->ClientTeamID());
						app->_DispatchMessage(NULL, B_QUIT_REQUESTED);
					}
				}
			}
			release_sem(fAppListLock);

			// When we delete the last ServerApp, we can exit the server
			fQuittingServer=true;
			fExitPoller=true;

			// also wait for picasso thread
			kill_thread(fPicassoThreadID);

			// poller thread is stuck reading messages from its input port
			// so, there is no cleaner way to make it quit, other than killing it!
			kill_thread(fPollerThreadID);

			// we are now clear to exit
			break;
		}
		case AS_SET_SYSCURSOR_DEFAULTS:
		{
			cursormanager->SetDefaults();
			break;
		}
		default:
			// we should never get here.
			printf("AppServer::DispatchMessage: Unhandled command");
			break;
	}
}

/*!
	\brief Send a quick (no attachments) message to all applications
	
	Quite useful for notification for things like server shutdown, system 
	color changes, etc.
*/
void AppServer::Broadcast(int32 code)
{
	ServerApp	*app= NULL;
	BMessage	msg(B_QUIT_REQUESTED);
	
	acquire_sem(fAppListLock);
	for(int32 i= 0; i < fAppList->CountItems(); i++)
	{
		app=(ServerApp*)fAppList->ItemAt(i);
		if(!app)
			{ printf("PANIC in AppServer::Broadcast()\n"); continue; }
		app->SendMessageToClient(&msg);
	}
	release_sem(fAppListLock);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void AppServer::R_ClientDied( thread_id hClient )
{
	ServerApp* tempapp = ServerApp::FindApp( hClient );

	if (tempapp)
		write_port(tempapp->_receiver, B_QUIT_REQUESTED, NULL, 0);
}

int AppServer::LoadWindowDecorator(const std::string& cPath)
{
	image_id oldDecoratorID = fDecoratorID;

	if (LoadDecorator(cPath.c_str()))
	{
		ServerApp::ReplaceDecorators();
		unload_add_on(oldDecoratorID);
		return(0);
	}
		
	return(-1);
}

/*!
	\brief Finds the application with the given signature
	\param sig MIME signature of the application to find
	\return the corresponding ServerApp or NULL if not found
	
	This call should be made only when necessary because it locks the app list 
	while it does its searching.
*/
ServerApp *AppServer::FindApp(const char *sig)
{
	if(!sig)
		return NULL;

	ServerApp *foundapp=NULL;

	acquire_sem(fAppListLock);

	for(int32 i=0; i<fAppList->CountItems();i++)
	{
		foundapp=(ServerApp*)fAppList->ItemAt(i);
		if(foundapp && foundapp->fSignature==sig)
		{
			release_sem(fAppListLock);
			return foundapp;
		}
	}

	release_sem(fAppListLock);
	
	// couldn't find a match
	return NULL;
}

/*!
	\brief Creates a new decorator instance
	\param rect Frame size
	\param title Title string for the "window"
	\param wlook Window look type. See Window.h
	\param wfeel Window feel type. See Window.h
	\param wflags Window flags. See Window.h
	
	If a decorator has not been set, we use the default one packaged in with the app_server 
	being that we can't do anything with a window without one.
*/
Decorator *new_decorator(Layer* pcView, int32 wlook, int32 wfeel, uint32 wflags)
{
	Decorator *dec=NULL;

	if(!app_server->make_decorator)
		dec=new DefaultDecorator(pcView, wlook, wfeel, wflags);
	else
		dec=app_server->make_decorator( pcView, wlook, wfeel, wflags );

	return dec;
}



/*!
	\brief Entry function to run the entire server
	\param argc Number of command-line arguments present
	\param argv String array of the command-line arguments
	\return -1 if the app_server is already running, 0 if everything's OK.
*/
int main( int argc, char** argv )
{
	STRACE(( "Appserver Alive %ld\n", find_thread(NULL) ));

	// There can be only one....
	if(find_port(SERVER_PORT_NAME)!=B_NAME_NOT_FOUND)
		return -1;

	STRACE(("There can be only one... app_server, that is.  We're it.\n"));

	g_pcLayers        = new Array<Layer>;

	AppserverConfig* pcConfig = new AppserverConfig();

	FILE* hFile;
	printf( "Load configuration\n" );

	const char* pzBaseDir = getenv( "COSMOE_SYS" );
	if( pzBaseDir == NULL )
	{
		pzBaseDir = "/cosmoe";
	}
	char* pzConfig = new char[ strlen(pzBaseDir) + 80 ];
	strcpy( pzConfig, pzBaseDir );
	strcat( pzConfig, "/config/appserver" );
	hFile = fopen( pzConfig, "r" );

	if ( hFile != NULL )
	{
		pcConfig->LoadConfig( hFile, false );
		fclose( hFile );
	}
	else
	{
		printf( "Error: failed to open appserver configuration file: %s\n", pzConfig );
	}
	delete[] pzConfig;

	AppServer	app_server;
	app_server.Run();
	return 0;
}
