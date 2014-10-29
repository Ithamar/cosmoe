//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Window.cpp
//	Author:			Adrian Oanca (adioanca@mymail.ro)
//	Description:	A BWindow object represents a window that can be displayed
//					on the screen, and that can be the target of user events
//------------------------------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <PropertyInfo.h>
#include <Handler.h>
#include <Looper.h>
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <String.h>
#include <Screen.h>
#include <Button.h>
#include <MessageQueue.h>
#include <MessageRunner.h>
#include <Roster.h>

// Project Includes ------------------------------------------------------------
#include <AppMisc.h>
#include <ServerProtocol.h>
#include <TokenSpace.h>
#include <MessageUtils.h>
#include <WindowAux.h>

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <math.h>

// Local Includes --------------------------------------------------------------
#include <Message.h>
#include <Bitmap.h>
#include <Menu.h>
#include <List.h>
#include <exceptions.h>
#include <macros.h>
#include <errno.h>


// Local Defines ---------------------------------------------------------------
//#define DEBUG_WIN
#ifdef DEBUG_WIN
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

// Globals ---------------------------------------------------------------------
static property_info windowPropInfo[] =
{
	{ "Feel", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the current feel of the window.",0 },

	{ "Feel", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the feel of the window.",0 },

	{ "Flags", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the current flags of the window.",0 },

	{ "Flags", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the window flags.",0 },

	{ "Frame", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the window's frame rectangle.",0},

	{ "Frame", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the window's frame rectangle.",0 },

	{ "Hidden", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if the window is hidden; false otherwise.",0},

	{ "Hidden", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Hides or shows the window.",0 },

	{ "Look", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the current look of the window.",0},

	{ "Look", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the look of the window.",0 },

	{ "Title", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns a string containing the window title.",0},

	{ "Title", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the window title.",0 },

	{ "Workspaces", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns int32 bitfield of the workspaces in which the window appears.",0},

	{ "Workspaces", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the workspaces in which the window appears.",0 },

	{ "MenuBar", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the key menu bar.",0 },

	{ "View", { 0 },
		{ 0 }, "Directs the scripting message to the top view without popping the current specifier.",0 },

	{ "Minimize", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Minimizes the window if \"data\" is true; restores otherwise.",0 },

	{ 0, { 0 }, { 0 }, 0, 0 }
}; 
//------------------------------------------------------------------------------
void _set_menu_sem_(BWindow *window, sem_id sem)
{
	window->fMenuSem=sem;
}


class TopView : public BView
{
public:
					TopView( const BRect& cFrame, BWindow* pcWindow );

	virtual void	FrameMoved( BPoint cDelta );
	virtual void	FrameResized( float inWidth, float inHeight );
private:
	BWindow*		m_pcWindow;
};

class BWindow::Private
{
public:
	Private()
	{
		m_psRenderPkt     = NULL;
		m_nRndBufSize     = RENDER_BUFFER_SIZE;
		m_hLayer          = -1;
		m_hLayerPort      = -1;
		m_nButtons        = 0;
		m_nMouseMoveRun   = 1;
		m_bDidScrollRect  = false;
	}

	WR_Render_s* m_psRenderPkt;
	uint32       m_nRndBufSize;

	int          m_nMouseMoveRun;
	int32        m_nMouseTransition;
	int          m_hLayer;
	port_id      m_hLayerPort;

	uint32       m_nButtons;
	BPoint       m_cMousePos;

	bool         m_bIsRunning;
	bool         m_bDidScrollRect;
};


void BWindow::_Cleanup()
{
	detachTopView();
	delete m->m_psRenderPkt;

	if ( receive_port >= 0 )
	{
		delete_port( receive_port );
	}

	delete m;
}


// Constructors
//------------------------------------------------------------------------------
BWindow::BWindow(BRect frame, const char* title, window_type type,
	uint32 flags,uint32 workspace)
	: BLooper( title, B_DISPLAY_PRIORITY )
{
	#ifdef DEBUG_WIN
		printf("BWindow::BWindow()\n");
	#endif
	window_look look;
	window_feel feel;
	
	decomposeType(type, &look, &feel);

	InitData( frame, title, look, feel, flags, workspace);
}

//------------------------------------------------------------------------------

BWindow::BWindow(BRect frame, const char* title, window_look look, window_feel feel,
	uint32 flags,uint32 workspace)
	: BLooper( title, B_DISPLAY_PRIORITY )
{
	InitData( frame, title, look, feel, flags, workspace );
}

/** Construct a window attached to a Bitmap
 * \par Description:
 *        This constructor is used internally by the GUI toolkit to create
 *        a window to handle rendering in a bitmap.
 * \param pcBitmap - The bitmap to which the window will be attached.
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

BWindow::BWindow( BBitmap* pcBitmap ) : BLooper( "bitmap", B_DISPLAY_PRIORITY )
{
	if ( be_app == NULL ){
		debugger("You need a valid BApplication object before interacting with the app_server");
		return;
	}

	m = new Private();
	SetTitle("bitmap_window");

	fFeel = B_NORMAL_WINDOW_FEEL;
	fLook = B_NO_BORDER_WINDOW_LOOK;
	fFlags = 0;

	try
	{
		receive_port = create_port( 15, "bwindow_reply" );

		BRect cWndBounds = pcBitmap->fBounds;

		top_view = new TopView( cWndBounds, this );

		int32 hTopView;
		m->m_hLayerPort = be_app->CreateWindow( top_view, pcBitmap->fToken, &hTopView );

		m->m_psRenderPkt             = new WR_Render_s;
		m->m_psRenderPkt->m_hTopView = hTopView;
		m->m_psRenderPkt->nCount     = 0;
		m->m_nRndBufSize             = 0;
		
		top_view->_Attached( this, NULL, hTopView, 0 );
	}
	catch(...)
	{
		_Cleanup();
		throw;
	}
}

//------------------------------------------------------------------------------

BWindow::BWindow(BMessage* data)
	: BLooper(data)
{
	BMessage		msg;
	const char		*title;
	window_look		look;
	window_feel		feel;

	data->FindString("_title", &title);
	data->FindInt32("_wlook", (int32*)&look);
	data->FindInt32("_wfeel", (int32*)&feel);

	if (data->FindInt64("_pulse", &fPulseRate) == B_OK )
		SetPulseRate( fPulseRate );
	//Unsupported
}

//------------------------------------------------------------------------------

BWindow::~BWindow()
{
	// the following lines, remove all existing shortcuts and delete accelList
	int32			noOfItems;
	
	noOfItems		= accelList.CountItems();
	for ( int index = noOfItems-1;  index >= 0; index-- ) {
		_BCmdKey		*cmdKey;

		cmdKey			= (_BCmdKey*)accelList.ItemAt( index );
		
		accelList.RemoveItem(index);
		
		delete cmdKey->message;
		delete cmdKey;
	}

	// TODO: release other dynamically-allocated objects

	// disable pulsing
	SetPulseRate( 0 );

	_Cleanup();
}

//------------------------------------------------------------------------------

BArchivable* BWindow::Instantiate(BMessage* data)
{
	if ( !validate_instantiation( data , "BWindow" ) ) 
		return NULL; 
	
	return new BWindow(data); 
}

//------------------------------------------------------------------------------

status_t BWindow::Archive(BMessage* data, bool deep) const{

	status_t		retval;

	retval		= BLooper::Archive( data, deep );
	if (retval != B_OK)
		return retval;

	data->AddString("_title", fTitle);
	data->AddInt32("_wlook", fLook);
	data->AddInt32("_wfeel", fFeel);
	if (fFlags)
		data->AddInt32("_flags", fFlags);
	data->AddInt32("_wspace", (uint32)Workspaces());

	if ( !composeType(fLook, fFeel) )
		data->AddInt32("_type", (uint32)Type());

	if (fMaxZoomWidth != 32768.0 || fMaxZoomHeight != 32768.0)
	{
		data->AddFloat("_zoom", fMaxZoomWidth);
		data->AddFloat("_zoom", fMaxZoomHeight);
	}

	if (fMinWindWidth != 0.0	 || fMinWindHeight != 0.0 ||
		fMaxWindWidth != 32768.0 || fMaxWindHeight != 32768.0)
	{
		data->AddFloat("_sizel", fMinWindWidth);
		data->AddFloat("_sizel", fMinWindHeight);
		data->AddFloat("_sizel", fMaxWindWidth);
		data->AddFloat("_sizel", fMaxWindHeight);
	}

	if (fPulseRate != 500000)
		data->AddInt64("_pulse", fPulseRate);

	if (deep)
	{
		int32		noOfViews = CountChildren();
		for (int i=0; i<noOfViews; i++){
			BMessage		childArchive;

			retval			= ChildAt(i)->Archive( &childArchive, deep );
			if (retval == B_OK)
				data->AddMessage( "_views", &childArchive );
		}
	}

	return B_OK;
}

//------------------------------------------------------------------------------

void BWindow::Quit(){

	if (!IsLocked())
	{
		const char* name = Name();
		if (!name)
			name = "no-name";

		printf("ERROR - you must Lock a looper before calling Quit(), "
			   "team=%ld, looper=%s\n", Team(), name);
	}

		// Try to lock
	if (!Lock()){
			// We're toast already
		return;
	}

	while (!IsHidden())	{ Hide(); }

	if (fFlags & B_QUIT_ON_WINDOW_CLOSE)
		be_app->PostMessage( B_QUIT_REQUESTED );

	BLooper::Quit();
}

//------------------------------------------------------------------------------

void BWindow::AddChild(BView *child, BView *before){
	top_view->AddChild( child, before );
}

//------------------------------------------------------------------------------

bool BWindow::RemoveChild(BView *child){
	return top_view->RemoveChild( child );
}

//------------------------------------------------------------------------------

int32 BWindow::CountChildren() const{
	return top_view->CountChildren();
}

//------------------------------------------------------------------------------

BView* BWindow::ChildAt(int32 index) const{
	return top_view->ChildAt( index );
}

//------------------------------------------------------------------------------

void BWindow::Minimize(bool minimize){
	if (IsModal())
		return;

	if (IsFloating())
		return;		

// FIXME: do minimize
}
//------------------------------------------------------------------------------

status_t BWindow::SendBehind(const BWindow* window){

	if (!window)
		return B_ERROR;

	return B_OK;
}

//------------------------------------------------------------------------------

void BWindow::Flush() const{
	if ( m->m_psRenderPkt->nCount == 0 )
	{
		return;
	}
	m->m_psRenderPkt->hReply = -1;
	if ( write_port( m->m_hLayerPort, WR_RENDER, m->m_psRenderPkt,
				sizeof( WR_Render_s ) + m->m_nRndBufSize - RENDER_BUFFER_SIZE ) )
	{
		printf( "Error: BWindow::Flush() failed to send WR_RENDER request to server\n" );
	}
	m->m_psRenderPkt->nCount = 0;
	m->m_nRndBufSize         = 0;
}

//------------------------------------------------------------------------------

void BWindow::Sync() const{
	m->m_psRenderPkt->hReply = receive_port;
	if ( write_port( m->m_hLayerPort, WR_RENDER, m->m_psRenderPkt,
				sizeof( WR_Render_s ) + m->m_nRndBufSize - RENDER_BUFFER_SIZE ) == 0 )
	{
		int32 msgCode;
		
		if ( read_port( receive_port, &msgCode, NULL, 0 ) < 0 )
		{
			printf( "Error: BWindow::Sync() failed to get reply\n" );
		}
	}
	m->m_psRenderPkt->nCount = 0;
	m->m_nRndBufSize         = 0;
}

//------------------------------------------------------------------------------

void BWindow::BeginViewTransaction()
{
	if ( !fInTransaction )
	{
		fInTransaction		= true;
	}
}

//------------------------------------------------------------------------------

void BWindow::EndViewTransaction()
{
	if ( fInTransaction )
	{
		fInTransaction		= false;		
	}
}

//------------------------------------------------------------------------------

bool BWindow::IsFront() const
{
	if (IsActive())
		return true;

	if (IsModal())
		return true;

	return false;
}

//------------------------------------------------------------------------------

void BWindow::MessageReceived( BMessage *msg )
{ 
	BMessage			specifier;
	int32				what;
	const char*			prop;
	int32				index;
	status_t			err;

	if (msg->HasSpecifiers()){

	err = msg->GetCurrentSpecifier(&index, &specifier, &what, &prop);
	if (err == B_OK)
	{
		BMessage			replyMsg;

		switch (msg->what)
		{
		case B_GET_PROPERTY:{
				replyMsg.what		= B_NO_ERROR;
				replyMsg.AddInt32( "error", B_OK );
				
				if (strcmp(prop, "Feel") ==0 )
				{
					replyMsg.AddInt32( "result", (uint32)Feel());
				}
				else if (strcmp(prop, "Flags") ==0 )
				{
					replyMsg.AddInt32( "result", Flags());
				}
				else if (strcmp(prop, "Frame") ==0 )
				{
					replyMsg.AddRect( "result", Frame());				
				}
				else if (strcmp(prop, "Hidden") ==0 )
				{
					replyMsg.AddBool( "result", IsHidden());				
				}
				else if (strcmp(prop, "Look") ==0 )
				{
					replyMsg.AddInt32( "result", (uint32)Look());				
				}
				else if (strcmp(prop, "Title") ==0 )
				{
					replyMsg.AddString( "result", Title());				
				}
				else if (strcmp(prop, "Workspaces") ==0 )
				{
					replyMsg.AddInt32( "result", Workspaces());				
				}
			}break;

		case B_SET_PROPERTY:{
				if (strcmp(prop, "Feel") ==0 )
				{
					uint32			newFeel;
					if (msg->FindInt32( "data", (int32*)&newFeel ) == B_OK){
						SetFeel( (window_feel)newFeel );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Flags") ==0 )
				{
					uint32			newFlags;
					if (msg->FindInt32( "data", (int32*)&newFlags ) == B_OK){
						SetFlags( newFlags );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Frame") ==0 )
				{
					BRect			newFrame;
					if (msg->FindRect( "data", &newFrame ) == B_OK){
						MoveTo( newFrame.LeftTop() );
						ResizeTo( newFrame.right, newFrame.bottom);
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Hidden") ==0 )
				{
					bool			newHiddenState;
					if (msg->FindBool( "data", &newHiddenState ) == B_OK){
						if ( !IsHidden() && newHiddenState == true ){
							Hide();
							
							replyMsg.what		= B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
							
						}
						else if ( IsHidden() && newHiddenState == false ){
							Show();
							
							replyMsg.what		= B_NO_ERROR;
							replyMsg.AddInt32( "error", B_OK );
						}
						else{
							replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
							replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
							replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
						}
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Look") ==0 )
				{
					uint32			newLook;
					if (msg->FindInt32( "data", (int32*)&newLook ) == B_OK){
						SetLook( (window_look)newLook );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
				else if (strcmp(prop, "Title") ==0 )
				{
					const char		*newTitle = NULL;
					if (msg->FindString( "data", &newTitle ) == B_OK){
						SetTitle( newTitle );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
					delete newTitle;
				}
				
				else if (strcmp(prop, "Workspaces") ==0 )
				{
					uint32			newWorkspaces;
					if (msg->FindInt32( "data", (int32*)&newWorkspaces ) == B_OK){
						SetWorkspaces( newWorkspaces );
						
						replyMsg.what		= B_NO_ERROR;
						replyMsg.AddInt32( "error", B_OK );
					}
					else{
						replyMsg.what		= B_MESSAGE_NOT_UNDERSTOOD;
						replyMsg.AddInt32( "error", B_BAD_SCRIPT_SYNTAX );
						replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
					}
				}
				
			}break;
		}
		msg->SendReply( &replyMsg );
	}
	else{
		BMessage		replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
		replyMsg.AddInt32( "error" , B_BAD_SCRIPT_SYNTAX );
		replyMsg.AddString( "message", "Didn't understand the specifier(s)" );
		
		msg->SendReply( &replyMsg );
	}

	} // END: if (msg->HasSpecifiers())
	else
		BLooper::MessageReceived( msg );
} 

//------------------------------------------------------------------------------

void BWindow::DispatchMessage(BMessage *msg, BHandler *target) 
{
	if (!msg){
		BLooper::DispatchMessage( msg, target );
		return;
	}

	BView*        pcView;

	if ( msg->FindPointer( "_widget", (void**) &pcView ) == 0 && pcView != NULL && find_handler( pcView->fToken ) == pcView )
	{
		switch( msg->what ) {
			case B_MOUSE_DOWN:
			{
				int32 nButtons;

				if ( msg->FindInt32( "buttons", &nButtons ) == 0 )
				{
					BView* pcChild;

					m->m_nButtons |= 1 << (nButtons-1);
					pcChild = FindView( top_view->ConvertFromScreen( m->m_cMousePos ) );
					if ( pcChild != NULL)
					{
						setFocus( pcChild );
						pcChild->MouseDown( pcChild->ConvertFromScreen( m->m_cMousePos ) );
					}
					else
					{
						BView* pcFocusView = CurrentFocus();
						if ( pcFocusView != NULL )
						{
							pcFocusView->MouseDown( pcFocusView->ConvertFromScreen( m->m_cMousePos ) );
						}
					}
				}
				else
				{
					printf( "Error: buttons not found\n" );
				}
				break;
			}

			case B_MOUSE_UP:
			{
				int32 nButtons;

				if ( msg->FindInt32( "buttons", &nButtons ) == 0 )
				{
					BMessage* pcData = NULL;
					BMessage  cData((uint32)0);

					BView*        pcChild;

					if ( msg->FindMessage( "_drag_message", &cData ) == 0 )
					{
						pcData = &cData;
					}

					m->m_nButtons &= ~( 1 << (nButtons - 1) );
					BPoint cPos = top_view->ConvertFromScreen( m->m_cMousePos );
					pcChild = FindView( cPos );
					
					if ( pcChild != NULL )
					{
						pcChild->MouseUp( pcChild->ConvertFromScreen( m->m_cMousePos )/*, pcData*/ );
					}
					
					if ( pcChild != CurrentFocus() )
					{
						pcChild = CurrentFocus();
						if ( pcChild != NULL )
						{
							pcChild->MouseUp( pcChild->ConvertFromScreen( m->m_cMousePos )/*, NULL*/ );
						}
					}
				}
				break;
			}

			case B_MOUSE_MOVED:
			{
				BMessageQueue*        pcQueue = MessageQueue();

				if ( msg->FindInt32( "_transit", &m->m_nMouseTransition ) != 0 ) {
					printf( "Warning: BWindow::DispatchMessage() could not find '_transit' in B_MOUSE_MOVED message\n" );
				}
				bool bDoProcess = true;

				pcQueue->Lock();
				BMessage* pcNextMsg = pcQueue->FindMessage( B_MOUSE_MOVED, 0 );
				if ( pcNextMsg != NULL ) {
					int32 nTransition;
					if ( msg->FindInt32( "_transit", &nTransition ) != 0 )
					{
						printf( "Warning: BWindow::DispatchMessage() could not find '_transit' in B_MOUSE_MOVED message\n" );
					}
					if ( nTransition == m->m_nMouseTransition )
					{
						bDoProcess = false;
					}
				}
				pcQueue->Unlock();
				if ( bDoProcess )         // Wait for the last mouse move message
				{
					if ( msg->FindPoint( "where", &m->m_cMousePos ) == 0 )
					{

						BMessage* pcData = NULL;
						BMessage  cData((uint32)0);

						if ( msg->FindMessage( "_drag_message", &cData ) == 0 )
						{
							pcData = &cData;
						}
						m->m_nMouseMoveRun++;
						_MouseEvent( m->m_cMousePos, m->m_nButtons, pcData, false );
					}
					else
					{
						printf( "ERROR : Could not find Point in mouse move message\n" );
					}
				}
				break;
			}
			
			case B_MOUSE_WHEEL_CHANGED:{
				if (fFocus)
					fFocus->MessageReceived( msg );
				break;}

			case B_KEY_DOWN:
			{
				int32              nQualifier;
				const char*        pzString;
				const char*        pzRawString;

				if ( msg->FindInt32( "modifiers", &nQualifier ) != 0 )
				{
					printf( "Error: BWindow::DispatchMessage() could not find 'modifiers' member in B_KEY_DOWN message\n" );
					break;
				}
				
				if ( msg->FindString( "_string", &pzString ) != 0 )
				{
					printf( "Error: BWindow::DispatchMessage() could not find '_string' member in B_KEY_DOWN message\n" );
					break;
				}
				
				if ( msg->FindString( "_raw_string", &pzRawString ) != 0 )
				{
					printf( "Error: BWindow::DispatchMessage() could not find '_raw_string' member in B_KEY_DOWN message\n" );
					break;
				}

				BView* pcFocusChild = CurrentFocus();

				if ( pcFocusChild != NULL )
				{
					if ( fDefaultButton != NULL && pzString[0] == B_ENTER && pzString[1] == '\0' )
					{
						fDefaultButton->KeyDown( pzString, strlen(pzString) );
						break;
					}

					pcFocusChild->KeyDown( pzString, strlen(pzString) );
				}
				break;
			}

			case B_KEY_UP:
			{
				BView* pcFocusChild = CurrentFocus();
				if ( pcFocusChild != NULL )
				{
					int32              nQualifier;
					const char*        pzString;
					const char*        pzRawString;

					if ( msg->FindInt32( "modifiers", &nQualifier ) != 0 )
					{
						printf( "Error: BWindow::DispatchMessage() could not find 'modifiers' member in B_KEY_UP message\n" );
						break;
					}
					if ( msg->FindString( "_string", &pzString ) != 0 )
					{
						printf( "Error: BWindow::DispatchMessage() could not find '_string' member in B_KEY_UP message\n" );
						break;
					}
					if ( msg->FindString( "_raw_string", &pzRawString ) != 0 )
					{
						printf( "Error: BWindow::DispatchMessage() could not find '_raw_string' member in B_KEY_UP message\n" );
						break;
					}
					if ( fDefaultButton != NULL && pzString[0] == B_ENTER && pzString[1] == '\0' ) {
						fDefaultButton->KeyUp( pzString, strlen(pzString) );
					} else {
						pcFocusChild->KeyUp( pzString, strlen(pzString) );
					}
				}
				break;
			}

			case B_QUIT_REQUESTED:{
				if (QuitRequested())
					Quit();
				break;}

			case _UPDATE_:{
				BRect        updateRect;
				if ( msg->FindRect( "frame", &updateRect ) == 0 )
				{
					pcView->_BeginUpdate();
					pcView->_ConstrictRectangle( &updateRect, BPoint(0,0) );
					pcView->EraseRect(updateRect);
					pcView->Draw(updateRect);
					pcView->_EndUpdate();
					if ( m->m_bDidScrollRect )
					{
						m->m_bDidScrollRect = false;
						Sync();
						//SpoolMessages();
					}
					else
					{
						Flush();
					}
				}
				else
				{
					printf( "Error: Could not find rectangle in paint message\n" );
				}
				break;
			}
			case M_FONT_CHANGED:
				pcView->FontChanged( pcView->GetFont() );
				break;

			default:
				BLooper::DispatchMessage( msg, target );
				break;
		}
	}
	else
	{
		switch( msg->what )
		{
			case B_PULSE:
				if (fPulseEnabled)
					sendPulse( top_view );
				break;

			case M_WINDOW_FRAME_CHANGED:
			{
				BRect cNewFrame;
				if ( msg->FindRect( "_new_frame", &cNewFrame ) != 0 )
				{
					printf( "Error: Could not find '_new_frame' in M_WINDOW_FRAME_CHANGED message\n" );
					break;
				}
				SetFrame( cNewFrame, false );
				if ( write_port( m->m_hLayerPort, WR_WND_MOVE_REPLY, NULL, 0 ) < 0 )
				{
					printf( "Error: BWindow::DispatchMessage() failed to send WR_WND_MOVE_REPLY to server\n" );
				}
				break;
			}
			case B_WINDOW_MOVE_BY:
			{
				BPoint cDeltaPos;
				BPoint cDeltaSize;

				if ( msg->FindPoint( "_delta_move", &cDeltaPos ) != 0 )
				{
					printf( "Error: Could not find '_delta_move' in B_WINDOW_MOVE_BY message\n" );
					break;
				}

				if ( msg->FindPoint( "_delta_size", &cDeltaSize ) != 0 )
				{
					printf( "Error: Could not find '_delta_size' in B_WINDOW_MOVE_BY message\n" );
					break;
				}
				BRect cFrame = Frame();

				cFrame.right  += cDeltaSize.x + cDeltaPos.x;
				cFrame.bottom += cDeltaSize.y + cDeltaPos.y;
				cFrame.left   += cDeltaPos.x;
				cFrame.top    += cDeltaPos.y;
				SetFrame( cFrame );
				Flush();
				if ( write_port( m->m_hLayerPort, WR_WND_MOVE_REPLY, NULL, 0 ) < 0 )
				{
					printf( "Error: BWindow::DispatchMessage() failed to send WR_WND_MOVE_REPLY to server\n" );
				}
				break;
			}
			
			case B_WINDOW_ACTIVATED:
			{
				bool        bIsActive;

				if ( msg->FindBool( "_is_active", &bIsActive ) != 0 )
				{
					printf( "Error: Failed to find '_is_active' member in B_WINDOW_ACTIVATED message\n" );
				}

				if ( msg->FindPoint( "where", &m->m_cMousePos ) != 0 )
				{
					printf( "Error: Failed to find 'where' member in B_WINDOW_ACTIVATED message\n" );
				}
				handleActivation( bIsActive, m->m_cMousePos );
				break;
			}

			default:
				BLooper::DispatchMessage(msg, target);
				break;
		}
	}
}


//------------------------------------------------------------------------------

void BWindow::FrameMoved(BPoint new_position)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::FrameResized(float new_width, float new_height)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::WorkspaceActivated(int32 ws, bool state)
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::MenusBeginning()
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::MenusEnded()
{
	// does nothing
	// Hook function
}

//------------------------------------------------------------------------------

void BWindow::SetSizeLimits(float minWidth, float maxWidth, 
							float minHeight, float maxHeight)
{

	if (minWidth > maxWidth)
		return;
	if (minHeight > maxHeight)
		return;

	Flush();
	if ( m->m_hLayerPort >= 0 )
	{
		BMessage cReq( AS_SET_SIZE_LIMITS );
		cReq.AddPoint( "min_size", BPoint(minWidth, minHeight));
		cReq.AddPoint( "max_size", BPoint(maxWidth, maxHeight));

		if ( BMessenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			printf( "Error: BWindow::SetSizeLimits() failed to send request to server\n" );
		}
		else
		{
			fMinWindHeight		= minHeight;
			fMinWindWidth		= minWidth;
			fMaxWindHeight		= maxHeight;
			fMaxWindWidth		= maxWidth;
		}
	}
}

//------------------------------------------------------------------------------

void BWindow::GetSizeLimits(float *minWidth, float *maxWidth, 
							float *minHeight, float *maxHeight)
{
	*minHeight			= fMinWindHeight;
	*minWidth			= fMinWindWidth;
	*maxHeight			= fMaxWindHeight;
	*maxWidth			= fMaxWindWidth;
}

//------------------------------------------------------------------------------

void BWindow::SetPulseRate(bigtime_t rate)
{
	if ( rate < 0 )
		return;

	fPulseRate = rate;
	if (rate == 0 || !fViewsNeedPulse)
	{
		fPulseEnabled = false;
		if (fPulseRunner)
			delete fPulseRunner;
		fPulseRunner = NULL;
	}
	else if (fViewsNeedPulse)
	{
		fPulseEnabled = true;
		if (fPulseRunner)
			fPulseRunner->SetInterval(rate);
		else
			fPulseRunner = new BMessageRunner( BMessenger(this), new BMessage( B_PULSE ), rate);
	}
}

//------------------------------------------------------------------------------

bigtime_t
BWindow::PulseRate() const
{
	return fPulseRate;
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMenuItem *item)
{
	AddShortcut(key, modifiers, item->Message(), this);
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage *msg)
{
	AddShortcut(key, modifiers, msg, this);
}


void
BWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage *msg, BHandler *target)
{
	// NOTE: I'm not sure if it is OK to use 'key'
	
	if (msg == NULL)
		return;

	int64				when;
	_BCmdKey			*cmdKey;

	when				= real_time_clock_usecs();
	msg->AddInt64("when", when);

	// TODO: make sure key is a lowercase char !!!

	modifiers			= modifiers | B_COMMAND_KEY;

	cmdKey				= new _BCmdKey;
	cmdKey->key			= key;
	cmdKey->modifiers	= modifiers;
	cmdKey->message		= msg;
	if (target == NULL)
		cmdKey->targetToken	= B_ANY_TOKEN;
	else
		cmdKey->targetToken	= _get_object_token_(target);

	// removes the shortcut from accelList if it exists!
	RemoveShortcut( key, modifiers );

	accelList.AddItem( cmdKey );

}

//------------------------------------------------------------------------------

void BWindow::RemoveShortcut(uint32 key, uint32 modifiers)
{
	int32				index;

	modifiers			= modifiers | B_COMMAND_KEY;

	index				= findShortcut( key, modifiers );
	if ( index >=0 ) 
	{
		_BCmdKey		*cmdKey;

		cmdKey			= (_BCmdKey*)accelList.ItemAt( index );

		accelList.RemoveItem(index);

		delete cmdKey->message;
		delete cmdKey;
	}
}

//------------------------------------------------------------------------------

BButton* BWindow::DefaultButton() const
{
	return fDefaultButton;
}

//------------------------------------------------------------------------------

void BWindow::SetDefaultButton(BButton* button)
{
/*
	Note: for developers:
	
	He he, if you really want to understand what is happens here, take a piece of
		paper and start taking possible values and then walk with them through
		the code.
*/
	BButton				*aux;

	if ( fDefaultButton == button )
		return;

	if ( fDefaultButton )
	{
		aux				= fDefaultButton;
		fDefaultButton	= NULL;
		aux->MakeDefault( false );
		aux->Invalidate();
	}
	
	if ( button == NULL )
	{
		fDefaultButton		= NULL;
		return;
	}
	
	fDefaultButton			= button;
	fDefaultButton->MakeDefault( true );
	fDefaultButton->Invalidate();
}

//------------------------------------------------------------------------------

BView* BWindow::FindView(const char* viewName) const{

	return findView( top_view, viewName );
}

//------------------------------------------------------------------------------

BView* BWindow::FindView(BPoint point) const
{

	BView*  pcView  = NULL;

	for ( BView* pcTmpWid = top_view->ChildAt( point ) ;
		  pcTmpWid != NULL ;
		  pcTmpWid = pcTmpWid->ChildAt( point ) )
	{
		BRect cChildFrame = pcTmpWid->Frame();

		pcView = pcTmpWid;

		point -= cChildFrame.LeftTop();
		point -= pcTmpWid->GetScrollOffset();
	}
	return pcView;
}

//------------------------------------------------------------------------------

BView* BWindow::CurrentFocus() const
{
	return fFocus;
}

//------------------------------------------------------------------------------

void BWindow::Activate(bool active)
{
	if (IsHidden())
		return;

	BMessage cReq( AS_ACTIVATE_WINDOW );

	cReq.AddBool( "focus", active );
	if ( BMessenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
	{
		printf( "Error: BWindow::Activate() failed to send AS_ACTIVATE_WINDOW to server\n" );
	}
}

//------------------------------------------------------------------------------

void BWindow::WindowActivated(bool state)
{
	// hook function
	// does nothing
}

//------------------------------------------------------------------------------

void BWindow::ConvertToScreen(BPoint* pt) const
{
	pt->x			+= Frame().left;
	pt->y			+= Frame().top;
}

//------------------------------------------------------------------------------

BPoint BWindow::ConvertToScreen(BPoint pt) const
{
	pt.x			+= Frame().left;
	pt.y			+= Frame().top;

	return pt;
}

//------------------------------------------------------------------------------

void BWindow::ConvertFromScreen(BPoint* pt) const
{
	pt->x			-= Frame().left;
	pt->y			-= Frame().top;
}

//------------------------------------------------------------------------------

BPoint BWindow::ConvertFromScreen(BPoint pt) const
{
	pt.x			-= Frame().left;
	pt.y			-= Frame().top;

	return pt;
}

//------------------------------------------------------------------------------

void BWindow::ConvertToScreen(BRect* rect) const
{
	rect->top			+= Frame().top;
	rect->left			+= Frame().left;
	rect->bottom		+= Frame().top;
	rect->right			+= Frame().left;
}

//------------------------------------------------------------------------------

BRect BWindow::ConvertToScreen(BRect rect) const
{
	rect.top			+= Frame().top;
	rect.left			+= Frame().left;
	rect.bottom			+= Frame().top;
	rect.right			+= Frame().left;

	return rect;
}

//------------------------------------------------------------------------------

void BWindow::ConvertFromScreen(BRect* rect) const
{
	rect->top			-= Frame().top;
	rect->left			-= Frame().left;
	rect->bottom		-= Frame().top;
	rect->right			-= Frame().left;
}

//------------------------------------------------------------------------------

BRect BWindow::ConvertFromScreen(BRect rect) const
{
	rect.top			-= Frame().top;
	rect.left			-= Frame().left;
	rect.bottom			-= Frame().top;
	rect.right			-= Frame().left;

	return rect;
}

//------------------------------------------------------------------------------

bool BWindow::IsMinimized() const
{
	// Hiding takes precendence over minimization!!!
	if ( IsHidden() )
		return false;

	return fMinimized;
}

//------------------------------------------------------------------------------

BRect BWindow::Bounds() const
{
	return( top_view->Bounds() );
}

//------------------------------------------------------------------------------

BRect BWindow::Frame() const
{
	return( top_view->Frame() );
}

//------------------------------------------------------------------------------

const char* BWindow::Title() const
{
	return fTitle;
}

//------------------------------------------------------------------------------

void BWindow::SetTitle(const char* title)
{
	if (!title)
		return;

	if (fTitle)
	{
		delete fTitle;
		fTitle = NULL;
	}
	
	fTitle = strdup( title );

	// we will change BWindow's thread name to "w>window_title"	
	int32		length;
	length		= strlen( fTitle );

	char		*threadName;
	threadName	= new char[32];
	strcpy(threadName, "w>");
	strncat(threadName, fTitle, (length>=29) ? 29: length);

	SetName(threadName);

	if ( m->m_hLayerPort >= 0 )
	{
		BMessage cReq( AS_WINDOW_TITLE );
		cReq.AddString( "title", fTitle );

		if ( BMessenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			printf( "Error: BWindow::SetTitle() failed to send request to server\n" );
		}
	}
}

//------------------------------------------------------------------------------

bool BWindow::IsActive() const
{
	return fActive || fWaitingForMenu;
}

//------------------------------------------------------------------------------

void BWindow::SetKeyMenuBar(BMenuBar* bar)
{
	fKeyMenuBar			= bar;
}

//------------------------------------------------------------------------------

BMenuBar* BWindow::KeyMenuBar() const
{
	return fKeyMenuBar;
}

//------------------------------------------------------------------------------

bool BWindow::IsModal() const
{
	if ( fFeel == B_MODAL_SUBSET_WINDOW_FEEL)
		return true;
	if ( fFeel == B_MODAL_APP_WINDOW_FEEL)
		return true;
	if ( fFeel == B_MODAL_ALL_WINDOW_FEEL)
		return true;

	return false;

}

//------------------------------------------------------------------------------

bool BWindow::IsFloating() const
{
	if ( fFeel == B_FLOATING_SUBSET_WINDOW_FEEL)
		return true;
	if ( fFeel == B_FLOATING_APP_WINDOW_FEEL)
		return true;
	if ( fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return true;

	return false;
}

//------------------------------------------------------------------------------

status_t BWindow::AddToSubset(BWindow* window)
{
	if ( !window )
			return B_ERROR;

	if (window->Feel() == B_MODAL_SUBSET_WINDOW_FEEL ||
		window->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL){
		return B_OK;
	}

	return B_ERROR;
}

//------------------------------------------------------------------------------

status_t BWindow::RemoveFromSubset(BWindow* window)
{
	if ( !window )
			return B_ERROR;

	return B_OK;
}

//------------------------------------------------------------------------------

status_t BWindow::Perform(perform_code d, void* arg)
{
	return BLooper::Perform( d, arg );
}

//------------------------------------------------------------------------------


status_t BWindow::SetType(window_type type)
{
	decomposeType(type, &fLook, &fFeel);
	status_t stat1, stat2;
	
	stat1=SetLook( fLook );
	stat2=SetFeel( fFeel );

	if(stat1==B_OK && stat2==B_OK)
		return B_OK;
	return B_ERROR;
}

//------------------------------------------------------------------------------

window_type	BWindow::Type() const
{
	return composeType( fLook, fFeel );
}

//------------------------------------------------------------------------------

status_t BWindow::SetLook(window_look look){

	// FIXME we don't actually make functional changes yet
	fLook = look;
	return B_OK;
}

//------------------------------------------------------------------------------

window_look	BWindow::Look() const
{
	return fLook;
}

//------------------------------------------------------------------------------

status_t BWindow::SetFeel(window_feel feel)
{

/* TODO:	See what happens when a window that is part of a subset, changes its
			feel!? should it be removed from the subset???
*/
	// FIXME we don't actually make functional changes yet
	fFeel = feel;
	return B_OK;
}

//------------------------------------------------------------------------------

window_feel	BWindow::Feel() const
{
	return fFeel;
}

//------------------------------------------------------------------------------

status_t BWindow::SetFlags(uint32 flags)
{
	Flush();
	if ( m->m_hLayerPort >= 0 )
	{
		BMessage cReq( AS_SET_FLAGS );
		cReq.AddInt32( "flags", flags );
		if ( BMessenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			printf( "Error: Window::SetFlags() failed to send request to server\n" );
			return B_ERROR;
		}
		else
		{
			fFlags = flags;
		}
	}

	return B_OK;
}

//------------------------------------------------------------------------------

uint32	BWindow::Flags() const{
	return fFlags;
}

//------------------------------------------------------------------------------

status_t BWindow::SetWindowAlignment(window_alignment mode,
											int32 h, int32 hOffset,
											int32 width, int32 widthOffset,
											int32 v, int32 vOffset,
											int32 height, int32 heightOffset)
{
	if ( !(	(mode && B_BYTE_ALIGNMENT) ||
			(mode && B_PIXEL_ALIGNMENT) ) )
	{
		return B_ERROR;
	}

	if ( 0 <= hOffset && hOffset <=h )
		return B_ERROR;

	if ( 0 <= vOffset && vOffset <=v )
		return B_ERROR;

	if ( 0 <= widthOffset && widthOffset <=width )
		return B_ERROR;

	if ( 0 <= heightOffset && heightOffset <=height )
		return B_ERROR;

    Flush();
    if ( m->m_hLayerPort >= 0 )
	{
        BMessage cReq( AS_SET_ALIGNMENT );
        cReq.AddIPoint( "size", IPoint(width, height) );
        cReq.AddIPoint( "size_off", IPoint(widthOffset, heightOffset) );
        cReq.AddIPoint( "pos", IPoint(h, v) );
        cReq.AddIPoint( "pos_off", IPoint(hOffset, vOffset) );

        if ( BMessenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
            printf( "Error: BWindow::SetWindowAlignment() failed to send request to server\n" );
        }
    }

	return B_OK;
}


/** Set the window's position and size.
 * \par Description:
 *        SetFrame() will set the window's client position and size on the
 *        current desktop.
 * \param cRect - The new frame rectangle of the window's client area.
 * \sa Frame(), Bounds(), MoveBy(), MoveTo(), ResizeBy(), ResizeTo()
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void BWindow::SetFrame( const BRect& cRect, bool bNotifyServer )
{
	top_view->SetFrame( cRect, bNotifyServer );
	Flush();
}

//------------------------------------------------------------------------------

uint32 BWindow::Workspaces() const
{
	return B_CURRENT_WORKSPACE;
}

//------------------------------------------------------------------------------

void BWindow::SetWorkspaces(uint32 workspaces)
{
}

//------------------------------------------------------------------------------

BView* BWindow::LastMouseMovedView() const
{
	return fLastMouseMovedView;
}

//------------------------------------------------------------------------------

void BWindow::MoveBy(float dx, float dy)
{
	top_view->MoveBy( dx, dy );
}

//------------------------------------------------------------------------------

void BWindow::MoveTo( BPoint point )
{
	MoveTo( point.x, point.y );
}

//------------------------------------------------------------------------------

void BWindow::MoveTo(float x, float y)
{
	top_view->MoveTo( x, y );
}

//------------------------------------------------------------------------------


void BWindow::ResizeBy(float dx, float dy)
{
	top_view->ResizeBy( dx, dy );
}

//------------------------------------------------------------------------------

void BWindow::ResizeTo(float width, float height)
{
		// stay in minimum & maximum frame limits
	width = (width < fMinWindWidth) ? fMinWindWidth : width;
	width = (width > fMaxWindWidth) ? fMaxWindWidth : width;

	height = (height < fMinWindHeight) ? fMinWindHeight : height;
	height = (height > fMaxWindHeight) ? fMaxWindHeight : height;

	top_view->ResizeTo( width, height );
}

//------------------------------------------------------------------------------

void BWindow::Show()
{
	if ( m->m_bIsRunning == false )
	{
		m->m_bIsRunning = true;
		Run();
	}

	fShowLevel--;
	top_view->Show();
}

//------------------------------------------------------------------------------

void BWindow::Hide()
{
	top_view->Hide( );
	if ( m->m_bIsRunning == false )
	{
		m->m_bIsRunning = true;
		Run();
	}
	fShowLevel++;
}


//------------------------------------------------------------------------------

bool BWindow::IsHidden() const
{
	return top_view->IsHidden();
}

//------------------------------------------------------------------------------

bool BWindow::QuitRequested()
{
	return BLooper::QuitRequested();
}

//------------------------------------------------------------------------------

thread_id BWindow::Run()
{
	return BLooper::Run();
}

//------------------------------------------------------------------------------

status_t BWindow::GetSupportedSuites(BMessage* data)
{
	status_t err = B_OK;
	if (!data)
		err = B_BAD_VALUE;

	if (!err)
	{
		err = data->AddString("Suites", "suite/vnd.Be-window");
		if (!err)
		{
			BPropertyInfo propertyInfo(windowPropInfo);
			err = data->AddFlat("message", &propertyInfo);
			if (!err)
				err = BLooper::GetSupportedSuites(data);
		}
	}
	return err;
}

//------------------------------------------------------------------------------

BHandler* BWindow::ResolveSpecifier(BMessage* msg, int32 index,	BMessage* specifier,
										int32 what,	const char* property)
{
	if (msg->what == B_WINDOW_MOVE_BY)
		return this;
	if (msg->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(windowPropInfo);
	switch (propertyInfo.FindMatch(msg, index, specifier, what, property))
	{
		case B_ERROR:
		{
			break;
		}
		
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		{
			return this;
		}
		
		case 14:
		{
			if (fKeyMenuBar)
			{
				msg->PopSpecifier();
				return fKeyMenuBar;
			}
			else
			{
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32( "error", B_NAME_NOT_FOUND );
				replyMsg.AddString( "message", "This window doesn't have a main MenuBar");
				msg->SendReply( &replyMsg );
				return NULL;
			}
		}
		
		case 15:
		{
			// we will NOT pop the current specifier
			return top_view;
		}
		
		case 16:
		{
			return this;
		}
	}

	return BLooper::ResolveSpecifier(msg, index, specifier, what, property);
}

// PRIVATE
//--------------------Private Methods-------------------------------------------
// PRIVATE

void BWindow::InitData(	BRect frame, const char* title, window_look look,
	window_feel feel, uint32 flags,	uint32 workspace)
{

	STRACE(("BWindow::InitData(...)\n"));
	
	fTitle=NULL;
	if ( be_app == NULL )
	{
		debugger("You need a valid BApplication object before interacting with the app_server");
		return;
	}

	m = new Private();
	
// TODO: what should I do if frame rect is invalid?
	if ( frame.IsValid() )
		fFrame			= frame;
	else
		frame.Set( frame.left, frame.top, frame.left+320, frame.top+240 );

	if (title)
		SetTitle( title );
	else
		SetTitle("no_name_window");
	
	fFeel			= feel;
	fLook			= look;
	fFlags			= flags;

	m->m_bIsRunning	= false;
	fInTransaction	= false;
	fActive			= false;
	fShowLevel		= 1;

	top_view		= NULL;
	fFocus			= NULL;
	fLastMouseMovedView	= NULL;
	fKeyMenuBar		= NULL;
	fDefaultButton	= NULL;

//	accelList		= new BList( 10 );
	AddShortcut('X', B_COMMAND_KEY, new BMessage(B_CUT), NULL);
	AddShortcut('C', B_COMMAND_KEY, new BMessage(B_COPY), NULL);
	AddShortcut('V', B_COMMAND_KEY, new BMessage(B_PASTE), NULL);
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), NULL);
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	fPulseEnabled	= false;
	fPulseRate		= 500000;
	fPulseRunner	= NULL;

	// TODO:  Remove the next two lines when BView learns to set these correctly
	fViewsNeedPulse = true;
	SetPulseRate(fPulseRate);

	// TODO:  see if you can use 'fViewsNeedPulse'

	fIsFilePanel	= false;

	// TODO: see WHEN is this used!
	fMaskActivated	= false;

	// TODO: see WHEN is this used!
	fWaitingForMenu	= false;

	fMinimized		= false;

	// TODO:  see WHERE you can use 'fMenuSem'

	fMaxZoomHeight	= 32768.0;
	fMaxZoomWidth	= 32768.0;
	fMinWindHeight	= 0.0;
	fMinWindWidth	= 0.0;
	fMaxWindHeight	= 32768.0;
	fMaxWindWidth	= 32768.0;

// TODO: other initializations!

	try
	{
/*
	Here, we will contact app_server and let him know that a window has
		been created
*/
		receive_port = create_port( 15, "window_reply" );
		if ( receive_port < 0 )
		{
			STRACE(("receive_port returned %ld!\n", receive_port));
			throw( GeneralFailure( "Failed to create message port", ENOMEM ) );
		}

		// Create and attach the top view
		top_view			= (TopView*)buildTopView();

		int32 hTopView;
		m->m_hLayerPort = be_app->CreateWindow( top_view, frame, title, look, feel, flags,
												workspace, fMsgPort, &hTopView );

		if ( m->m_hLayerPort < 0 )
		{
			STRACE(("m->m_hLayerPort returned %ld!\n", m->m_hLayerPort));
			throw( GeneralFailure( "Failed to open window", ENOMEM ) );
		}

		m->m_psRenderPkt				= new WR_Render_s;
		m->m_psRenderPkt->m_hTopView	= hTopView;
		m->m_psRenderPkt->nCount		= 0;
		m->m_nRndBufSize				= 0;

		top_view->_Attached( this, NULL, hTopView, 1 );
//        Lock();
	}
	catch(...)
	{
		_Cleanup();
		throw;
	}
}


//------------------------------------------------------------------------------

window_type BWindow::composeType(window_look look,	
								 window_feel feel) const
{
	window_type returnValue = B_UNTYPED_WINDOW;

	switch(feel)
	{
		case B_NORMAL_WINDOW_FEEL:
		{
			switch (look)
			{
				case B_TITLED_WINDOW_LOOK:
				{
					returnValue = B_TITLED_WINDOW;
					break;
				}
				case B_DOCUMENT_WINDOW_LOOK:
				{
					returnValue = B_DOCUMENT_WINDOW;
					break;
				}
				case B_BORDERED_WINDOW_LOOK:
				{
					returnValue = B_BORDERED_WINDOW;
					break;
				}
				default:
				{
					returnValue = B_UNTYPED_WINDOW;
				}
			}
			
			break;
		}
		case B_MODAL_APP_WINDOW_FEEL:
		{
			if (look == B_MODAL_WINDOW_LOOK)
				returnValue = B_MODAL_WINDOW;
			break;
		}
		case B_FLOATING_APP_WINDOW_FEEL:
		{
			if (look == B_FLOATING_WINDOW_LOOK)
				returnValue = B_FLOATING_WINDOW;
			break;
		}	
		default:
		{
			returnValue = B_UNTYPED_WINDOW;
		}
	}

	return returnValue;
}

//------------------------------------------------------------------------------

void BWindow::decomposeType(window_type type, window_look* look,
		window_feel* feel) const
{
	switch (type)
	{
		case B_TITLED_WINDOW:
		{
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		case B_DOCUMENT_WINDOW:
		{
			*look = B_DOCUMENT_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		case B_MODAL_WINDOW:
		{
			*look = B_MODAL_WINDOW_LOOK;
			*feel = B_MODAL_APP_WINDOW_FEEL;
			break;
		}
		case B_FLOATING_WINDOW:
		{
			*look = B_FLOATING_WINDOW_LOOK;
			*feel = B_FLOATING_APP_WINDOW_FEEL;
			break;
		}
		case B_BORDERED_WINDOW:
		{
			*look = B_BORDERED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		case B_UNTYPED_WINDOW:
		{
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
		default:
		{
			*look = B_TITLED_WINDOW_LOOK;
			*feel = B_NORMAL_WINDOW_FEEL;
			break;
		}
	}
}

//------------------------------------------------------------------------------

BView* BWindow::buildTopView()
{
 	STRACE(("BuildTopView(): enter\n"));
	BView			*topView;

	topView		= new TopView( fFrame, this );

	topView->top_level_view	= true;

	// set top_view's owner, add it to window's eligible handler list
	// and also set its next handler to be this window.

	topView->owner			= this;

 	//we can't use AddChild() because this is the top_view
	topView->attached		= true;

/* Note:
		I don't think adding top_view to BLooper's list
		of eligible handlers is a good idea!
*/
  
 	STRACE(("BuildTopView ended\n"));

	return topView;
}

//------------------------------------------------------------------------------

void BWindow::detachTopView(){

// TODO: detach all views

	if ( top_view != NULL )
	{
		BMessage cReq( AS_QUIT_WINDOW );

		cReq.AddInt32( "top_view", top_view->_GetHandle() );

		top_view->_Detached( true, 0 );
		delete top_view;
		if ( BMessenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			printf( "Error: Window::_Cleanup() failed to send AR_CLOSE_WINDOW request to server\n" );
		}
	}
}

//------------------------------------------------------------------------------

void BWindow::prepareView(BView *aView)
{
	// TODO: implement
}

//------------------------------------------------------------------------------

void BWindow::attachView(BView *aView)
{
	// TODO: implement
}

//------------------------------------------------------------------------------

void BWindow::detachView(BView *aView)
{
	// TODO: implement
}
//------------------------------------------------------------------------------


void BWindow::setFocus(BView *focusView, bool notifyInputServer)
{
	BView* previousFocus = fFocus;

	if (previousFocus == focusView)
		return;

	SetPreferredHandler(focusView);

	if (previousFocus != NULL)
		previousFocus->MakeFocus(false);

	fFocus			= focusView;

	if (focusView != NULL)
		focusView->MakeFocus(true);

	// TODO: find out why do we have to notify input server.
	if (notifyInputServer)
	{
		// what am I suppose to do here??
	}
}

//------------------------------------------------------------------------------

void BWindow::_ViewDeleted( BView* pcView )
{
	if ( pcView == fLastMouseMovedView )
	{
		fLastMouseMovedView = NULL;
	}

	if ( pcView == fDefaultButton )
	{
		fDefaultButton = NULL;
	}
}



void BWindow::handleActivation(bool active, const BPoint& cMousePos )
{
	fActive = active;

	if ( NULL == top_view )
	{
		printf( "Error: BWindow::handleActivation() top_view == NULL\n" );
	}

	WindowActivated( active );
	top_view->_WindowActivated( fActive );

	m->m_nMouseTransition = B_INSIDE_VIEW;
	_MouseEvent( cMousePos, m->m_nButtons, NULL, false ); // FIXME: Get buttons from message
}



void* BWindow::_AllocRenderCmd( uint32 nCmd, BView* pcView, uint32 nSize )
{
	void* pObj = NULL;

	if ( nSize <= RENDER_BUFFER_SIZE )
	{
		if ( m->m_nRndBufSize + nSize > RENDER_BUFFER_SIZE )
		{
			Flush();
		}
		pObj = (void*) &m->m_psRenderPkt->aBuffer[ m->m_nRndBufSize ];

		GRndHeader_s* psHdr = (GRndHeader_s*) pObj;

		psHdr->nSize      = nSize;
		psHdr->nCmd       = nCmd;
		psHdr->hViewToken = pcView->_GetHandle();

		m->m_psRenderPkt->nCount++;
		m->m_nRndBufSize += nSize;

		if ( nCmd == DRC_COPY_RECT )
		{
			m->m_bDidScrollRect = true;
		}
	}
	else
	{
		printf( "Error: BView::_AllocRenderCmd() packet to big!\n" );
	}
	return( pObj );
}


void BWindow::_CallMouseMoved( BView* pcView, uint32 nButtons, int nWndTransit, const BMessage* pcData )
{
	while ( pcView != NULL && pcView->_GetMouseMoveRun() == m->m_nMouseMoveRun )
	{
		pcView = pcView->Parent();
	}

	if ( pcView == NULL )
	{
		return;
	}
	
	BPoint cPos = pcView->ConvertFromScreen( m->m_cMousePos );
	int    nCode;

	if ( (nWndTransit == B_ENTERED_VIEW || nWndTransit == B_INSIDE_VIEW) &&
		pcView->Bounds().Contains( cPos ) )
	{
		if ( pcView->_GetMouseMode() == B_OUTSIDE_VIEW )
		{
			nCode = B_ENTERED_VIEW;
			pcView->_SetMouseMode( B_INSIDE_VIEW );
		}
		else
		{
			nCode = B_INSIDE_VIEW;
		}
	}
	else
	{
		if ( pcView->_GetMouseMode() == B_INSIDE_VIEW )
		{
			nCode = B_EXITED_VIEW;
			pcView->_SetMouseMode( B_OUTSIDE_VIEW );
		}
		else
		{
			nCode = B_OUTSIDE_VIEW;
		}
	}

	pcView->_SetMouseMoveRun( m->m_nMouseMoveRun );
	pcView->MouseMoved( cPos, nCode, pcData );
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BWindow::_MouseEvent( const BPoint& cNewPos, uint32 nButtons, const BMessage* pcData, bool bReEntered )
{
	BView* pcFocusChild;
	BView* pcMouseView;

	pcMouseView = FindView( top_view->ConvertFromScreen( m->m_cMousePos ) );
	pcFocusChild = CurrentFocus();

	if ( fLastMouseMovedView != pcMouseView )
	{
		if ( fLastMouseMovedView != NULL )
		{
			_CallMouseMoved( fLastMouseMovedView, nButtons, m->m_nMouseTransition, pcData );
		}
		fLastMouseMovedView = pcMouseView;
	}

	if ( pcMouseView != NULL )
	{
		_CallMouseMoved( pcMouseView, nButtons, m->m_nMouseTransition, pcData );
	}

	if ( pcFocusChild != NULL )
	{
		_CallMouseMoved( pcFocusChild, nButtons, m->m_nMouseTransition, pcData );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
BHandler* BWindow::find_handler(int32 token)
{
	BHandler* handler = NULL;

	for (int32 j = 0; j < CountHandlers(); j++)
	{
		if (HandlerAt(j)->fToken == token)
		{
			handler = HandlerAt(j);
			break;
		}
	}

	return handler;
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
//        Send a delete request to the appserver to get rid of the server
//        part of a view object. When the object is deleted on the server
//        we ask the BLooper to copy all unread messages into the internal
//        message queue and then filter out all messages that was targeted
//        to the delete view.
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BWindow::_DeleteViewFromServer( BView* pcView )
{
	Sync();
	if ( pcView != top_view )
	{
		BMessage cReq( AS_LAYER_DELETE );

		cReq.AddInt32( "top_view", top_view->_GetHandle() );
		cReq.AddInt32( "handle", pcView->_GetHandle() );

		if ( BMessenger( m->m_hLayerPort ).SendMessage( &cReq ) < 0 )
		{
			printf( "Error: BWindow::_DeleteViewFromServer() failed to send message to server\n" );
		}
	}

//	SpoolMessages(); // Copy all messages from the message port to the message queue.
	BMessageQueue*        pcQueue = MessageQueue();
	BMessageQueue cTmp;
	BMessage*     pcMsg;

	pcQueue->Lock();

	while( (pcMsg = pcQueue->NextMessage()) != NULL )
	{
		BView* pcTmpView;
		if ( pcMsg->FindPointer( "_widget", (void**) &pcTmpView ) == 0 && pcTmpView == pcView ) {
			delete pcMsg;
		}
		else
		{
			cTmp.AddMessage( pcMsg );
		}
	}

	while( (pcMsg = cTmp.NextMessage()) != NULL )
	{
		pcQueue->AddMessage( pcMsg );
	}

	pcQueue->Unlock();
}


int BWindow::ToggleDepth()
{
	return( top_view->ToggleDepth() );
}


BPoint BWindow::_GetPenPosition( int nViewHandle )
{
	WR_GetPenPosition_s      sReq;
	WR_GetPenPositionReply_s sReply;
	int32 msgCode;

	sReq.m_hTopView    = top_view->_GetHandle();
	sReq.m_hReply      = receive_port;
	sReq.m_hViewHandle = nViewHandle;

	if ( write_port( m->m_hLayerPort, WR_GET_PEN_POSITION, &sReq, sizeof(sReq) ) == 0 )
	{
		if ( read_port( receive_port, &msgCode, &sReply, sizeof(sReply) ) < 0 )
		{
			printf( "Error: BWindow::_GetPenPosition() failed to get reply\n" );
		}
	}
	else
	{
		printf( "Error: BWindow::_GetPenPosition() failed to send request to server\n" );
	}

	return(sReply.m_cPos);
}




void BWindow::ScreenChanged(BRect screen_size, color_space depth)
{
}




TopView::TopView( const BRect& cFrame, BWindow* pcWindow ) :
    BView( cFrame, "window_top_view", B_FOLLOW_ALL, B_WILL_DRAW )
{
	m_pcWindow = pcWindow;
}


void TopView::FrameMoved( BPoint cDelta )
{
	m_pcWindow->FrameMoved( cDelta );
}


void TopView::FrameResized( float inWidth, float inHeight )
{
	m_pcWindow->FrameResized( inWidth, inHeight );
}


void BWindow::_DefaultColorsChanged()
{
}


void BWindow::_SetMenuOpen( bool bOpen )
{
	fWaitingForMenu = bOpen;
}


BView* BWindow::_GetTopView() const
{
	return( top_view );
}


port_id BWindow::_GetAppserverPort() const
{
	return( m->m_hLayerPort );
}

void BWindow::UpdateIfNeeded()
{
	Sync();
}

//------------------------------------------------------------------------------

void BWindow::sendPulse( BView* aView )
{
	BView *child; 
	if ( (child = aView->first_child) )
	{
		while ( child )
		{ 
			if ( child->Flags() & B_PULSE_NEEDED )
				child->Pulse();
			sendPulse( child );
			child = child->next_sibling; 
		}
	}
}

//------------------------------------------------------------------------------

int32 BWindow::findShortcut( uint32 key, uint32 modifiers )
{
	int32			index,
					noOfItems;

	index			= -1;
	noOfItems		= accelList.CountItems();

	for ( int32 i = 0;  i < noOfItems; i++ )
	{
		_BCmdKey*		tempCmdKey;

		tempCmdKey		= (_BCmdKey*)accelList.ItemAt(i);
		if (tempCmdKey->key == key && tempCmdKey->modifiers == modifiers)
		{
			index		= i;
			break;
		}
	}
	
	return index;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, int32 token)
{

	if ( _get_object_token_(aView) == token )
		return aView;

	BView			*child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			BView*		view;
			if ( (view = findView( child, token )) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, const char* viewName) const
{

	if ( strcmp( viewName, aView->Name() ) == 0)
		return aView;

	BView			*child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			BView*		view;
			if ( (view = findView( child, viewName )) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findView(BView* aView, BPoint point) const
{

	if ( aView->Bounds().Contains(point) && !aView->first_child )
		return aView;

	BView			*child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			BView*		view;
			if ( (view = findView( child, point )) )
				return view;
			child 		= child->next_sibling; 
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

BView* BWindow::findNextView( BView *focus, uint32 flags)
{
	bool		found;
	found		= false;

	BView		*nextFocus;
	nextFocus	= focus;

	// Ufff... this toked me some time... this is the best form I've reached.
	// This algorithm searches the tree for BViews that accept focus.
	while (!found)
	{
		if (nextFocus->first_child)
			nextFocus		= nextFocus->first_child;
		else
		{
			if (nextFocus->next_sibling)
				nextFocus		= nextFocus->next_sibling;
			else
			{
				while( !nextFocus->next_sibling && nextFocus->parent )
					nextFocus		= nextFocus->parent;

				if (nextFocus == top_view)
					nextFocus		= nextFocus->first_child;
				else
					nextFocus		= nextFocus->next_sibling;
			}
		}
		
		if (nextFocus->Flags() & flags)
			found = true;

		// It means that the hole tree has been searched and there is no
		// view with B_NAVIGABLE_JUMP flag set!
		if (nextFocus == focus)
			return NULL;
	}

	return nextFocus;
}

//------------------------------------------------------------------------------

BView* BWindow::findPrevView( BView *focus, uint32 flags)
{
	bool		found;
	found		= false;

	BView		*prevFocus;
	prevFocus	= focus;

	BView		*aView;

	while (!found)
	{
		if ( (aView = findLastChild(prevFocus)) )
			prevFocus		= aView;
		else
		{
			if (prevFocus->prev_sibling)
				prevFocus		= prevFocus->prev_sibling;
			else
			{
				while( !prevFocus->prev_sibling && prevFocus->parent )
					prevFocus		= prevFocus->parent;

				if (prevFocus == top_view)
					prevFocus		= findLastChild( prevFocus );
				else
					prevFocus		= prevFocus->prev_sibling;
			}
		}
		
		if (prevFocus->Flags() & flags)
			found = true;


		// It means that the hole tree has been searched and there is no
		// view with B_NAVIGABLE_JUMP flag set!
		if (prevFocus == focus)
			return NULL;
	}

	return prevFocus;
}

//------------------------------------------------------------------------------

BView* BWindow::findLastChild(BView *parent)
{
	BView		*aView;
	if ( (aView = parent->first_child) )
	{
		while (aView->next_sibling)
			aView		= aView->next_sibling;

		return aView;
	}
	else
		return NULL;
}

//------------------------------------------------------------------------------

void BWindow::DoUpdate(BView* aView, BRect& area)
{

 	STRACE(("info: BWindow::drawView() BRect(%f,%f,%f,%f) called.\n",
 		area.left, area.top, area.right, area.bottom));
	aView->Draw( area );

	BView *child;
	if ( (child = aView->first_child) )
	{
		while ( child )
		{
			if ( area.Intersects( child->Frame() ) )
			{
				BRect		newArea;

				newArea		= area & child->Frame();
				child->ConvertFromParent( &newArea );
				child->Invalidate( newArea );
			}
			child 		= child->next_sibling; 
		}
	}
}

//------------------------------------------------------------------------------

void BWindow::SetIsFilePanel(bool yes)
{
	// TODO: is this not enough?
	fIsFilePanel	= yes;
}

//------------------------------------------------------------------------------

bool BWindow::IsFilePanel() const
{
	return fIsFilePanel;
}

//------------------------------------------------------------------------------
// Virtual reserved Functions

void BWindow::_ReservedWindow1() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow2() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow3() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow4() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow5() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow6() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow7() { }
//------------------------------------------------------------------------------
void BWindow::_ReservedWindow8() { }


/*
TODO list:

	*) take care of temporarely events mask!!!
	*) what's with this flag B_ASYNCHRONOUS_CONTROLS ?
	*) test arguments for SetWindowAligment
	*) call hook functions: MenusBeginning, MenusEnded. Add menu activation code.
*/

/*
 @log
*/
