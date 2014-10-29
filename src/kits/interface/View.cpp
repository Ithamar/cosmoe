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
//	File Name:		View.cpp
//	Author:			Adrian Oanca <adioanca@myrealbox.com>
//	Description:   A BView object represents a rectangular area within a window.
//					The object draws within this rectangle and responds to user
//					events that are directed at the window.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <PropertyInfo.h>
#include <Handler.h>
#include <View.h>
#include <Window.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Rect.h>
#include <Point.h>
#include <Region.h>
#include <Font.h>
#include <ScrollBar.h>
#include <Cursor.h>
#include <Bitmap.h>
#include <Polygon.h>
#include <Shape.h>
#include <Button.h>
#include <Shelf.h>
#include <String.h>
#include <SupportDefs.h>
#include <Application.h>

// Project Includes ------------------------------------------------------------
#include <AppMisc.h>
#include <TokenSpace.h>
#include <MessageUtils.h>
#include <ServerProtocol.h>

// Local Includes --------------------------------------------------------------
#include <stdio.h>
#include <assert.h>
#include <macros.h>

// Local Defines ---------------------------------------------------------------

//#define DEBUG_BVIEW
#ifdef DEBUG_BVIEW
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_BVIEW
#	define BVTRACE PrintToStream()
#else
#	define BVTRACE ;
#endif

#define MAX_ATTACHMENT_SIZE 49152

inline rgb_color _get_rgb_color( uint32 color );
inline uint32 _get_uint32_color( rgb_color c );
inline rgb_color _set_static_rgb_color( uint8 r, uint8 g, uint8 b, uint8 a=255 );
inline void _set_ptr_rgb_color( rgb_color* c, uint8 r, uint8 g, uint8 b, uint8 a=255 );
inline bool _rgb_color_are_equal( rgb_color c1, rgb_color c2 );
inline bool _is_new_pattern( const pattern& p1, const pattern& p2 );

// Globals ---------------------------------------------------------------------
static property_info viewPropInfo[] =
{
	{ "Frame", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the view's frame rectangle.",0 },

	{ "Frame", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Sets the view's frame rectangle.",0 },

	{ "Hidden", { B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns true if the view is hidden; false otherwise.",0 },

	{ "Hidden", { B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Hides or shows the view.",0 },

	{ "Shelf", { 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Directs the scripting message to the shelf.",0 },

	{ "View", { B_COUNT_PROPERTIES, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, "Returns the number of of child views.",0 },

	{ "View", { 0 },
		{ B_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_REVERSE_INDEX_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ "View", { 0 },
		{ B_NAME_SPECIFIER, 0 }, "Directs the scripting message to the specified view.",0 },

	{ 0, { 0 }, { 0 }, 0, 0 }
}; 


// General Functions
//------------------------------------------------------------------------------

int BView::s_nNextTabOrder = 0;

static rgb_color g_asDefaultColors[] =
{
	{ 0xaa, 0xaa, 0xaa, 0xff },        // COL_NORMAL
	{ 0xff, 0xff, 0xff, 0xff },        // COL_SHINE
	{ 0x00, 0x00, 0x00, 0xff },        // COL_SHADOW
	{ 0x66, 0x88, 0xbb, 0xff },        // COL_SEL_WND_BORDER
	{ 0x78, 0x78, 0x78, 0xff },        // COL_NORMAL_WND_BORDER
	{ 0x00, 0x00, 0x00, 0xff },        // COL_MENU_TEXT
	{ 0x00, 0x00, 0x00, 0xff },        // COL_SEL_MENU_TEXT
	{ 0xcc, 0xcc, 0xcc, 0xff },        // COL_MENU_BACKGROUND
	{ 0x66, 0x88, 0xbb, 0xff },        // COL_SEL_MENU_BACKGROUND
	{ 0x78, 0x78, 0x78, 0xff },        // COL_SCROLLBAR_BG
	{ 0xaa, 0xaa, 0xaa, 0xff },        // COL_SCROLLBAR_KNOB
	{ 0x78, 0x78, 0x78, 0xff },        // COL_LISTVIEW_TAB
	{ 0xff, 0xff, 0xff, 0xff }         // COL_LISTVIEW_TAB_TEXT
};


typedef struct StateStruct
{
	BPoint			origin;	// FIXME: origin saved/restored but not implemented
	float			scale;	// FIXME: scale saved/restored but not implemented
	drawing_mode	drawingMode;

// Line Cap/Join Mode FIXME
// Miter limit FIXME
// Pen size FIXME

	BPoint			penLocation;
	rgb_color		highColor;
	rgb_color   	lowColor;

// stipple pattern FIXME
// Local/Global clipping regions FIXME
// Font context FIXME

	StateStruct* nextState;
} StateStruct;


typedef struct LineArrayElement
{
	BPoint			from;
	BPoint			to;
	rgb_color		color;
} LineArrayElement;


/** Get the value of one of the standard system colors.
 * \par Description:
 *        Call this function to obtain one of the user-configurable
 *        system colors. This should be used whenever possible instead
 *        of hardcoding colors to make it possible for the user to
 *        customize the look of the operating system.
 * \par Note:
 * \par Warning:
 * \param
 *        nColor - One of the COL_xxx enums from default_color_t
 * \return
 *        The current color for the given system pen.
 * \sa
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

rgb_color get_default_color( default_color_t nColor )
{
	return( g_asDefaultColors[nColor] );
}

void __set_default_color( default_color_t nColor, const rgb_color& sColor )
{
	g_asDefaultColors[nColor] = sColor;
}

void set_default_color( default_color_t nColor, const rgb_color& sColor )
{
	g_asDefaultColors[nColor] = sColor;
}

static rgb_color Tint( const rgb_color& sColor, float vTint )
{
	int r = int( (float(sColor.red) * vTint + 127.0f * (1.0f - vTint)) );
	int g = int( (float(sColor.green) * vTint + 127.0f * (1.0f - vTint)) );
	int b = int( (float(sColor.blue) * vTint + 127.0f * (1.0f - vTint)) );
	if ( r < 0 ) r = 0; else if (r > 255) r = 255;
	if ( g < 0 ) g = 0; else if (g > 255) g = 255;
	if ( b < 0 ) b = 0; else if (b > 255) b = 255;
	rgb_color ret = {r, g, b, sColor.alpha};
	return ret;
}


class BView::Private
{
public:
	port_id      m_hReplyPort;      // Used as reply address when talking to server
	BView*       m_pcBottomChild;

	BView*       m_pcPrevFocus;

	BRect        m_cFrame;
	float        mScale;
	BPoint       m_cScrollOffset;
	std::string  m_cTitle;

	drawing_mode m_eDrawingMode;
	rgb_color    m_sFgColor;
	rgb_color    m_sBgColor;
	rgb_color    m_sEraseColor;

	BFont*        m_pcFont;
	StateStruct* mStateStack;

	int          m_nBeginPaintCount;

	uint32       m_nResizeMask;
	uint32       m_nFlags;
	int          m_nMouseMoveRun;
	int          m_nMouseMode;        /* Record whether the mouse was inside or outside
								* the bounding box during the last mouse event */
	int				m_nTabOrder; // Sorting order for keyboard manouvering
	
	int					mLineArrayAllocation;
	int					mLineArrayCount;
	LineArrayElement*	mLineArray;
};


// Constructors
//------------------------------------------------------------------------------

BView::BView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BHandler( name ),

origin_h(0),
origin_v(0),
owner(NULL),
parent(NULL),
next_sibling(NULL),
prev_sibling(NULL),
first_child(NULL),
fShowLevel(0),
fVerScroller(NULL),
fHorScroller(NULL)
{
	m = new Private;

	m->m_hReplyPort = create_port( 15, "view_reply" );

	m->m_cFrame					= frame;
	m->m_cTitle					= name ? name : "";
	m->m_nResizeMask			= resizingMode;
	m->m_nFlags					= flags;

	m->m_pcPrevFocus			= NULL;

	m->m_pcBottomChild			= NULL;

	server_token				= -1;

	m->m_pcFont					= NULL;
	m->m_nBeginPaintCount		= 0;

	m->m_sFgColor.red			= 0;
	m->m_sFgColor.green			= 0;
	m->m_sFgColor.blue			= 0;
	m->m_sFgColor.alpha			= 255;

	m->m_eDrawingMode			= B_OP_COPY;
	m->m_nMouseMode				= B_OUTSIDE_VIEW;
	m->m_nMouseMoveRun			= 0;
	m->mStateStack				= NULL;

	m->m_nTabOrder				= -1;
	m->m_sBgColor				= ui_color( B_PANEL_BACKGROUND_COLOR );
	m->m_sEraseColor			= ui_color( B_PANEL_BACKGROUND_COLOR );

	m->mScale					= 1.0f;

	m->mLineArrayAllocation		= 0;
	m->mLineArrayCount			= 0;
	m->mLineArray				= NULL;

	BFont* pcFont = new BFont();

	font_properties sProp;
	be_app->GetDefaultFont( DEFAULT_FONT_REGULAR, &sProp );

	if ( pcFont->SetFamilyAndStyle( sProp.m_cFamily.c_str(), sProp.m_cStyle.c_str() ) == 0 )
	{
		pcFont->SetProperties( sProp.m_vSize, sProp.m_vShear, sProp.m_vRotation );
		SetFont( pcFont );
		pcFont->Release();
		Flush();
	}
	else
	{
		printf( "Error : BView::View() unable to create font\n" );
	}
}

//---------------------------------------------------------------------------

BView::BView(BMessage *archive)
	: BHandler( archive )
{
	// FIXME: stub so that we can compile in BArchivable functionality
}

//---------------------------------------------------------------------------

BArchivable *BView::Instantiate(BMessage *data)
{
   if ( !validate_instantiation( data , "BView" ) ) 
      return NULL; 
   return new BView(data); 
}

//---------------------------------------------------------------------------

status_t BView::Archive(BMessage* data, bool deep) const{
	status_t		retval;

	retval		= BHandler::Archive( data, deep );
	if (retval != B_OK)
		return retval;

	return retval;
}



//---------------------------------------------------------------------------

BView::~BView()
{
STRACE(("BView(%s)::~BView()\n", this->Name()));

	BView* pcChild;

	if (fVerScroller)
		fVerScroller->SetTarget( (const char*)NULL );

	if (fHorScroller)
		fHorScroller->SetTarget( (const char*)NULL );

	BWindow* pcWnd = Window();

	if ( pcWnd != NULL )
	{
		pcWnd->_ViewDeleted( this );
	}

	while( (pcChild = first_child) /*  GetChildAt( 0 ) */ )
	{
		RemoveChild( pcChild );
		delete pcChild;
	}

	if ( parent != NULL )
	{
		parent->RemoveChild( this );
	}
	_ReleaseFont();
	delete_port( m->m_hReplyPort );
	delete m;
}

//---------------------------------------------------------------------------

BRect BView::Bounds() const
{
	BRect bounds(m->m_cFrame);
	bounds.OffsetTo(0,0);
	bounds.OffsetBy(-m->m_cScrollOffset.x, -m->m_cScrollOffset.y);
	return bounds;
}

//---------------------------------------------------------------------------

void BView::ConvertToParent(BPoint* pt) const
{
	*pt += LeftTop() + m->m_cScrollOffset;
}

//---------------------------------------------------------------------------

BPoint BView::ConvertToParent(BPoint pt) const
{
	return( pt + LeftTop() + m->m_cScrollOffset );
}

//---------------------------------------------------------------------------

void BView::ConvertFromParent(BPoint* pt) const
{
	*pt -= LeftTop() + m->m_cScrollOffset;
}

//---------------------------------------------------------------------------

BPoint BView::ConvertFromParent(BPoint pt) const
{
	return( pt - LeftTop() - m->m_cScrollOffset );
}

//---------------------------------------------------------------------------

void BView::ConvertToParent(BRect* r) const
{
	r->OffsetBy(LeftTop() + m->m_cScrollOffset);
}

//---------------------------------------------------------------------------

BRect BView::ConvertToParent(BRect r) const
{
	return( r.OffsetBySelf(LeftTop() + m->m_cScrollOffset) );
}

//---------------------------------------------------------------------------

void BView::ConvertFromParent(BRect* r) const
{
	r->OffsetBy(-m->m_cFrame.left, -m->m_cFrame.top);
	r->OffsetBy(-m->m_cScrollOffset.x, -m->m_cScrollOffset.y);
}

//---------------------------------------------------------------------------

BRect BView::ConvertFromParent(BRect r) const
{
	r.OffsetBy(-m->m_cFrame.left, -m->m_cFrame.top);
	r.OffsetBy(-m->m_cScrollOffset.x, -m->m_cScrollOffset.y);

	return( r );
}

//---------------------------------------------------------------------------



void BView::ConvertToScreen(BPoint* pt) const
{
	*pt += LeftTop() + m->m_cScrollOffset;

	if ( parent != NULL )
	{
		parent->ConvertToScreen( pt );
	}
}

//---------------------------------------------------------------------------

BPoint BView::ConvertToScreen(BPoint pt) const
{
	if ( parent != NULL )
		return( parent->ConvertToScreen( pt + LeftTop() + m->m_cScrollOffset ) );

	return( pt + LeftTop() + m->m_cScrollOffset );
}

//---------------------------------------------------------------------------

void BView::ConvertFromScreen(BPoint* pt) const
{
	*pt -= LeftTop() + m->m_cScrollOffset;

	if ( parent != NULL )
	{
		parent->ConvertFromScreen( pt );
	}
}

//---------------------------------------------------------------------------

BPoint BView::ConvertFromScreen(BPoint pt) const
{
	if ( parent != NULL )
		return( parent->ConvertFromScreen( pt - LeftTop() - m->m_cScrollOffset ) );

	return( pt - LeftTop() );
}

//---------------------------------------------------------------------------


void BView::ConvertToScreen(BRect* r) const
{
	r->OffsetBy(LeftTop() + m->m_cScrollOffset);

	if ( parent != NULL )
		parent->ConvertToScreen( r );
}

//---------------------------------------------------------------------------

BRect BView::ConvertToScreen(BRect r) const
{
	if ( parent != NULL )
		return( parent->ConvertToScreen(r.OffsetBySelf(LeftTop() + m->m_cScrollOffset) ) );

	return(r.OffsetBySelf(LeftTop()));
}

//---------------------------------------------------------------------------

void BView::ConvertFromScreen(BRect* r) const
{
	r->OffsetBy(-m->m_cFrame.left, -m->m_cFrame.top);
	r->OffsetBy(-m->m_cScrollOffset.x, -m->m_cScrollOffset.y);

	if ( parent != NULL )
	{
		parent->ConvertFromScreen( r );
	}
}

//---------------------------------------------------------------------------

BRect BView::ConvertFromScreen(BRect r) const
{
	r.OffsetBy(-m->m_cFrame.left, -m->m_cFrame.top);

	if ( parent != NULL )
		return( parent->ConvertFromScreen( r.OffsetBySelf(-m->m_cScrollOffset) ) );

	return( r );
}

//---------------------------------------------------------------------------

uint32 BView::Flags(uint32 nMask) const 
{
	return( m->m_nFlags & nMask );
}

//---------------------------------------------------------------------------

void BView::SetFlags( uint32 flags )
{
/*	Some useful info:
		fFlags is a unsigned long (32 bits)
		* bits 1-16 are used for BView's flags
		* bits 17-32 are used for BView' resize mask
		* _RESIZE_MASK_ is used for that. Look into View.h to see how
			it's defined
*/
	m->m_nFlags = flags;
}

//---------------------------------------------------------------------------

BRect BView::Frame() const 
{
	return m->m_cFrame;
}

//---------------------------------------------------------------------------

void BView::Hide()
{
	fShowLevel++;
	show_view(false);
}

//---------------------------------------------------------------------------

void BView::Show()
{
	fShowLevel--;
	show_view(true);
}

//---------------------------------------------------------------------------

bool BView::IsFocus() const 
{
	BWindow* win = Window();

	if ( NULL != win )
	{
		if ( win->IsActive() == false )
			return false;

		BView* focus = win->CurrentFocus();
		return (focus == this);
	}

	return false;
}

//---------------------------------------------------------------------------

bool 
BView::IsHidden(const BView *lookingFrom) const
{
	if (fShowLevel > 0)
		return true;

	// FIXME: ignores lookingFrom

	return false;
}

//---------------------------------------------------------------------------

bool
BView::IsHidden() const
{
	return IsHidden(NULL);
}

//---------------------------------------------------------------------------

bool BView::IsPrinting() const 
{
	return f_is_printing;
}

//---------------------------------------------------------------------------

BPoint BView::LeftTop() const 
{
	return(m->m_cFrame.LeftTop());
}


//---------------------------------------------------------------------------

void BView::SetOrigin(BPoint pt) 
{
	SetOrigin( pt.x, pt.y );
}

//---------------------------------------------------------------------------

void BView::SetOrigin(float x, float y) 
{
	// TODO: maybe app_server should do a redraw? - WRITE down into specs

	origin_h = x;
	origin_v = y;
}

//---------------------------------------------------------------------------

BPoint BView::Origin(void) const
{
	return BPoint(origin_h, origin_v);
}

//---------------------------------------------------------------------------

void BView::SetResizingMode(uint32 mode) 
{
	m->m_nResizeMask = mode;
}

//---------------------------------------------------------------------------

uint32 BView::ResizingMode() const {
	return( m->m_nResizeMask );
}

//---------------------------------------------------------------------------

void BView::SetViewCursor(const BCursor *cursor, bool sync)
{

	if (!cursor)
		return;

	if (!owner)
		debugger("View method requires owner and doesn't have one");

	BMessage cReq( AS_LAYER_CURSOR );
	cReq.AddInt32("token", cursor->m_serverToken);
	if ( BMessenger( owner->_GetAppserverPort() ).SendMessage(&cReq) < 0 )
	{
		printf( "BView::SetViewCursor() failed to send request to server\n" );
	}
}

//---------------------------------------------------------------------------

void BView::Flush(void) const 
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
		pcWindow->Flush();
}

//---------------------------------------------------------------------------

void BView::Sync(void) const 
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
		pcWindow->Sync();
}

//---------------------------------------------------------------------------

BWindow* BView::Window() const 
{
	return( (BWindow*)Looper() );
}



// Hook Functions
//---------------------------------------------------------------------------

void BView::AttachedToWindow()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AttachedToWindow()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::AllAttached()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AllAttached()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::DetachedFromWindow()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::DetachedFromWindow()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::AllDetached()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::AllDetached()\n", Name()));
}

//---------------------------------------------------------------------------

void BView::Draw(BRect updateRect)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::Draw()\n", Name()));
}

void BView::DrawAfterChildren(BRect r)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::DrawAfterChildren()\n", Name()));
}

void BView::FrameMoved(BPoint new_position)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::FrameMoved()\n", Name()));
}

void BView::FrameResized(float new_width, float new_height)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::FrameResized()\n", Name()));
}

void BView::GetPreferredSize(float* width, float* height)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::GetPreferredSize()\n", Name()));
	if (width)
		*width = 50.0f;

	if (height)
		*height = 20.0f;
}

void BView::ResizeToPreferred()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::ResizeToPreferred()\n", Name()));

	float width, height;

	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void BView::KeyDown(const char* bytes, int32 numBytes)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::KeyDown()\n", Name()));
// FIXME: I believe the BeBook specifically says NOT to do this

	if ( parent != NULL )
	{
		parent->KeyDown(bytes, numBytes);
	}
}

void BView::KeyUp(const char* bytes, int32 numBytes)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::KeyUp()\n", Name()));
// FIXME: I believe the BeBook specifically says NOT to do this

	if ( parent != NULL )
		parent->KeyUp(bytes, numBytes);
}

void BView::MouseDown(BPoint where)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseDown()\n", Name()));
	// FIXME: BeOS did not send to parent automatically

	if ( parent != NULL )
	{
		parent->MouseDown( ConvertToParent( where ) );
	}
}

void BView::MouseUp(BPoint where)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseUp()\n", Name()));
	// FIXME: BeOS did not send to parent automatically

	if ( parent != NULL )
	{
		parent->MouseUp( ConvertToParent( where ) );
	}
}

void BView::MouseMoved(BPoint where, uint32 code, const BMessage* a_message)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::MouseMoved()\n", Name()));

	BWindow* pcWnd = Window();

	if ( pcWnd != NULL )
	{
		int32 nButton = 0x01;

		pcWnd->CurrentMessage()->FindInt32("buttons", &nButton);

		pcWnd->_MouseEvent( ConvertToScreen(where), nButton, a_message, true );
	}
}

void BView::Pulse()
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::Pulse()\n", Name()));
}

void BView::TargetedByScrollView(BScrollView* scroll_view)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::TargetedByScrollView()\n", Name()));
}

void BView::WindowActivated(bool state)
{
	// HOOK function
	STRACE(("\tHOOK: BView(%s)::WindowActivated()\n", Name()));
}

// Input Functions
//---------------------------------------------------------------------------

void BView::BeginRectTracking(BRect startRect, uint32 style)
{
}

//---------------------------------------------------------------------------

void BView::EndRectTracking()
{
}

//---------------------------------------------------------------------------

void BView::SetViewColor(rgb_color c)
{
	BWindow* pcWindow = Window();

	m->m_sEraseColor = c;

	if ( pcWindow != NULL )
	{
		GRndSetrgb_color* psCmd = static_cast<GRndSetrgb_color*>(pcWindow->_AllocRenderCmd( DRC_SET_COLOR32, this, sizeof( GRndSetrgb_color ) ));
		if ( psCmd != NULL )
		{
			psCmd->nWhichPen = PEN_ERASE;
			psCmd->sColor    = c;
		}
	}
}

rgb_color BView::ViewColor() const
{
	return( m->m_sEraseColor );
}

void BView::SetHighColor( rgb_color a_color )
{
	BWindow* pcWindow = Window();
	m->m_sFgColor = a_color;

	if ( pcWindow != NULL )
	{
		GRndSetrgb_color* psCmd = static_cast<GRndSetrgb_color*>(pcWindow->_AllocRenderCmd( DRC_SET_COLOR32, this, sizeof( GRndSetrgb_color ) ));
		if ( psCmd != NULL )
		{
			psCmd->nWhichPen     = PEN_HIGH;
			psCmd->sColor        = a_color;
		}
	}
}

rgb_color BView::HighColor() const
{
	return( m->m_sFgColor );
}

void BView::SetLowColor( rgb_color a_color )
{
	BWindow* pcWindow = Window();

	m->m_sBgColor = a_color;

	if ( pcWindow != NULL )
	{
		GRndSetrgb_color* psCmd = static_cast<GRndSetrgb_color*>(pcWindow->_AllocRenderCmd( DRC_SET_COLOR32, this, sizeof( GRndSetrgb_color ) ));
		if ( psCmd != NULL )
		{
			psCmd->nWhichPen     = PEN_LOW;
			psCmd->sColor        = a_color;
		}
	}
}

rgb_color BView::LowColor() const
{
	return( m->m_sBgColor );
}

/** Get the keybord manouvering order.
 * \par Description:
 *        This member is called by the system to decide which view to select next
 *        when the <TAB> key is pressed. The focus is given to the next view with
 *        higher or equal tab-order as the current. You can overload this member
 *        to decide the order whenever it is called, or rely on the default
 *        implementation that will return whatever was set by SetTabOrder().
 *        A negative return value means that the view should not be skipped when
 *        searching for the next view to activate.
 *
 * \return The views sorting order for keyboard manouvering.
 * \sa SetTabOrder()
 * \author        Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

int BView::GetTabOrder()
{
	return( m->m_nTabOrder );
}

/** Set the keyboard manouvering sorting order.
 * \par Description:
 *        Set the value that will be returned by GetTabOrder().
 * \param
 *        nOrder - The sorting order.
 * \sa GetTabOrder()
 * \author        Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////

void BView::SetTabOrder( int nOrder )
{
	m->m_nTabOrder = nOrder;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::_LinkChild( BView* pcChild, bool bTopmost )
{
	if ( NULL == pcChild->parent )
	{
		pcChild->parent        = this;
		if ( NULL == m->m_pcBottomChild && NULL == first_child )
		{
			m->m_pcBottomChild        = pcChild;
			first_child        = pcChild;
			pcChild->next_sibling         = NULL;
			pcChild->prev_sibling = NULL;
		}
		else
		{
			if ( bTopmost )
			{
				if ( NULL != first_child )
				{
					first_child->prev_sibling = pcChild;
				}

				pcChild->next_sibling = first_child;
				first_child = pcChild;
				pcChild->prev_sibling = NULL;
			}
			else
			{
				if ( NULL != m->m_pcBottomChild )
				{
					m->m_pcBottomChild->next_sibling = pcChild;
				}
				pcChild->prev_sibling = m->m_pcBottomChild;
				m->m_pcBottomChild                   = pcChild;
				pcChild->next_sibling  = NULL;
			}
		}
	}
	else
	{
		printf( "ERROR : Attempt to add an view already belonging to a window\n" );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::_UnlinkChild( BView* pcChild )
{
	if( pcChild->parent == this )
	{
		pcChild->parent = NULL;

		if ( pcChild == first_child )
		{
			first_child = pcChild->next_sibling;
		}

		if ( pcChild == m->m_pcBottomChild )
		{
			m->m_pcBottomChild = pcChild->prev_sibling;
		}

		if ( NULL != pcChild->next_sibling )
		{
			pcChild->next_sibling->prev_sibling = pcChild->prev_sibling;
		}

		if ( NULL != pcChild->prev_sibling )
		{
			pcChild->prev_sibling->next_sibling = pcChild->next_sibling;
		}

		pcChild->next_sibling  = NULL;
		pcChild->prev_sibling = NULL;
	}
	else
	{
		printf( "ERROR : Attempt to remove a view not belonging to this window\n" );
	}
}


/** Start a drag and drop operation.
 * \par Description:
 *
 *        This member is normally called from the MouseMoved() member to
 *        initiate a drag and drop operation. The function takes a
 *        BMessage containing the data to drag, and information about
 *        how to visually represent the dragged data to the user. The
 *        visual representation can be either a bitmap (possibly with
 *        an alpha-channel to make it half-transparent) or a simple
 *        rectangle.
 *
 *        When a drag and drop operation is in progress the application
 *        server will include the message given here in \p pcData in
 *        B_MOUSE_MOVED (BView::MouseMoved()) messages sent to views
 *        as the user moves the mouse and also in the B_MOUSE_UP
 *        (BView::MouseUp()) message that terminate the operation.
 * 
 * \param pcData
 *        A BMessage object containing the data to be dragged.
 * \param cHotSpot
 *        Mouse position relative to the visible representation
 *        of the dragged object.
 * \param pcBitmap
 *        A bitmap that will be used as the visible representation
 *        of the dragged data. The bitmap can have an alpha channel.
 * \param pcReplyHandler
 *        The handler that should receive replies sent by the receiver
 *        of the dragged data.
 * \sa MouseMoved(), MouseUp(), BMessage, BHandler
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void BView::DragMessage( BMessage* pcData, BBitmap* pcBitmap, BPoint offset, BHandler* reply_to )
{
	BWindow* pcWindow = Window();
	if ( pcWindow == NULL )
	{
		printf( "Error: BView::BeginDrag() called on view not attached to a window\n" );
		return;
	}

	BMessage cReq( WR_BEGIN_DRAG );

	if (!reply_to)
		reply_to = this;

	_set_message_reply_( pcData, BMessenger( reply_to, reply_to->Looper() ) );
	//pcData->SetReplyHandler( (reply_to==NULL) ? this : reply_to );

	cReq.AddInt32( "bitmap", pcBitmap->fToken );
	cReq.AddPoint( "hot_spot", offset );
	cReq.AddMessage( "data", pcData );

	BMessage cReply;
	if ( BMessenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
	{
		printf( "BView::BeginDrag() failed to send request to server\n" );
	}
}


void BView::DragMessage(BMessage* pcData, BPoint offset, BRect cBounds, BHandler* reply_to )
{
	BWindow* pcWindow = Window();
	if ( pcWindow == NULL )
	{
		printf( "Error: BView::BeginDrag() called on view not attached to a window\n" );
		return;
	}

	BMessage cReq( WR_BEGIN_DRAG );

	if (!reply_to)
		reply_to = this;

	_set_message_reply_( pcData, BMessenger( reply_to, reply_to->Looper() ) );
	//pcData->SetReplyHandler( (reply_to==NULL) ? this : reply_to );

	cReq.AddInt32( "bitmap", -1 );
	cReq.AddRect( "bounds", cBounds );
	cReq.AddPoint( "hot_spot", offset );
	cReq.AddMessage( "data", pcData );

	BMessage cReply;
	if ( BMessenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
	{
		printf( "BView::BeginDrag() failed to send request to server\n" );
	}
}

void BView::GetMouse(BPoint* location,uint32 *buttons,bool checkMessageQueue)
{
	BWindow* pcWindow = Window();
	if ( pcWindow == NULL )
	{
		printf( "Error: BView::GetMouse() called on view not attached to a window\n" );
		return;
	}

	BMessage cReq( WR_GET_MOUSE );

	BMessage cReply;
	if ( BMessenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
	{
		printf( "BView::GetMouse() failed to send request to server\n" );
	}

	if ( location != NULL )
	{
		cReply.FindPoint( "position", location );
		ConvertFromScreen( location );
	}

	if ( buttons != NULL )
	{
		cReply.FindInt32( "buttons", (int32*)buttons );
	}
}



BPoint BView::GetContentSize() const
{
	BRect cBounds = Frame();
	return( BPoint( cBounds.Width(), cBounds.Height() ) );
}

void BView::SetDrawingMode(drawing_mode mode)
{
	if ( mode != m->m_eDrawingMode )
	{
		BWindow* pcWnd = Window();
		m->m_eDrawingMode = mode;

		if ( pcWnd == NULL )
		{
			return;
		}

		GRndSetDrawingMode_s* psMsg;
		psMsg = static_cast<GRndSetDrawingMode_s*>( pcWnd->_AllocRenderCmd( DRC_SET_DRAWING_MODE, this,
																		sizeof( GRndSetDrawingMode_s ) ) );
		if ( psMsg != NULL )
		{
			psMsg->nDrawingMode = mode;
		}
	}
}

drawing_mode BView::DrawingMode() const
{
	return( m->m_eDrawingMode );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BPoint BView::GetScrollOffset( void ) const
{
	return( m->m_cScrollOffset );
}




// Notify this BView and all descendants with AttachedToWindow().
// Parent views are notified before children views.
void BView::_NotifyAttachedToWindow()
{
	// Invoke AttachedToWindow() for this view, and then notify all children.
	AttachedToWindow();
	for ( BView* pcChild = m->m_pcBottomChild ; NULL != pcChild ; pcChild = pcChild->prev_sibling )
	{
		pcChild->_NotifyAttachedToWindow();
	}
}

// Notify this BView and all descendants with AllAttached().
// Children views are notified before parent views.
void BView::_NotifyAllAttached()
{
	// Invoke AllAttached() for all the children first and then invoke for this view.
	for ( BView* pcChild = m->m_pcBottomChild ; NULL != pcChild ; pcChild = pcChild->prev_sibling )
	{
		pcChild->_NotifyAllAttached();
	}
	AllAttached();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::_Detached( bool bFirst, int nHideCount )
{
	BView* pcChild;

	fShowLevel -= nHideCount;

	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		DetachedFromWindow();
	}

	for ( int i = 0 ; (pcChild = ChildAt( i )) ; ++i )
	{
		pcChild->_Detached( false, nHideCount );
	}

	if ( pcWindow != NULL )
	{
		AllDetached();
	}

	if ( pcWindow != NULL && server_token != -1 )
	{
		pcWindow->Flush();
		pcWindow->_DeleteViewFromServer( this );
		server_token = -1;
		if ( pcWindow->RemoveHandler( this ) == false )
		{
			printf( "BView::_Detached() failed to remove view from Looper\n" );
		}
		pcWindow->_ViewDeleted( this );
	}

	if ( bFirst )
	{
		parent = NULL;
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::_WindowActivated( bool bIsActive )
{
	BWindow* pcWnd = Window();
	BView*   pcChild;

	assert( pcWnd != NULL );

	WindowActivated( bIsActive );
	if ( pcWnd->CurrentFocus() == this )
	{
		MakeFocus( bIsActive );
	}

	for ( pcChild = m->m_pcBottomChild ; NULL != pcChild ; pcChild = pcChild->prev_sibling )
	{
		pcChild->_WindowActivated( bIsActive );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::_IncHideCount( bool bVisible )
{
	if ( bVisible )
	{
		fShowLevel--;
	}
	else
	{
		fShowLevel++;
	}

	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndShowView_s* psCmd;
		psCmd = (GRndShowView_s*) pcWindow->_AllocRenderCmd( DRC_SHOW_VIEW, this, sizeof(GRndShowView_s) );

		if ( psCmd != NULL )
		{
			psCmd->bVisible = bVisible;
		}
	}

	for ( BView* pcChild = m->m_pcBottomChild ; pcChild != NULL ; pcChild = pcChild->prev_sibling )
	{
		pcChild->_IncHideCount( bVisible );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------



void BView::SetMousePos( const BPoint& cPosition )
{
	BWindow* pcWindow = Window();
	if ( pcWindow == NULL )
	{
		printf( "Error: BView::GetMouse() called on view not attached to a window\n" );
		return;
	}

	BMessage cReq( WR_SET_MOUSE_POS );

	cReq.AddPoint( "position", ConvertToScreen( cPosition ) );

	if ( BMessenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq ) < 0 )
	{
		printf( "BView::SetMousePos() failed to send request to server\n" );
	}
}


void BView::WheelMoved( BPoint& inPoint )
{
}

int BView::ToggleDepth( void )
{
	BWindow* pcWindow = Window();

	if ( NULL != pcWindow )
	{
		BMessage cReq( WR_TOGGLE_VIEW_DEPTH );

		cReq.AddInt32( "top_view", pcWindow->_GetTopView()->server_token );
		cReq.AddInt32( "view", server_token );

		BMessage cReply;
		if ( BMessenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply ) < 0 )
		{
			printf( "Error: Window::ToggleDepth() failed to send WR_TOGGLE_VIEW_DEPTH request to the server\n" );
		}
	}

	if ( NULL != parent )
	{
		if ( parent->first_child != this )
		{
			parent->_UnlinkChild( this );
			parent->_LinkChild( this, true );
		}
		else
		{
			parent->_UnlinkChild( this );
			parent->_LinkChild( this, false );
		}
	}
	return( false );
}

/** Set the size and position relative to the parent view.
 * \par Description:
 *        Set the frame-rectangle of the view relative to its parent.
 *        If this cause new areas of this view or any of its
 *        siblings/children the affected views will receive a draw
 *        message to update the newly exposed areas.
 *
 *        If the upper-left corner of the view is moved the
 *        BView::FrameMoved() hook will be called and if the lower-right
 *        corner move relative to the upper-left corner the
 *        BView::FrameResized() hook will be called.
 *
 * \param cRect
 *        The new frame rectangle.
 * \sa ResizeTo(), ResizeBy(), MoveTo(), MoveBy()
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void BView::SetFrame( const BRect& cRect, bool bNotifyServer )
{
	BPoint cMove = BPoint( cRect.left, cRect.top ) - BPoint( m->m_cFrame.left, m->m_cFrame.top );
	BPoint cSize = BPoint( cRect.Width(), cRect.Height() ) - BPoint( m->m_cFrame.Width(), m->m_cFrame.Height() );

	m->m_cFrame = cRect;

	BWindow* pcWindow = Window();

	if ( cSize.x || cSize.y || cMove.x || cMove.y )
	{
		if ( bNotifyServer && pcWindow != NULL )
		{
			GRndSetFrame_s* psCmd = static_cast<GRndSetFrame_s*>(pcWindow->_AllocRenderCmd( DRC_SET_FRAME, this, sizeof(GRndSetFrame_s) ));

			if ( psCmd != NULL )
			{
				psCmd->cFrame = cRect;
			}
		}

		if ( cSize.x || cSize.y )
		{
			BView* pcChild;

			for ( pcChild = m->m_pcBottomChild ; NULL != pcChild ; pcChild = pcChild->prev_sibling )
			{
				pcChild->_ParentSized( cSize );
			}
		}
	}

	if ( cSize.x || cSize.y )
		FrameResized( cSize.x, cSize.y );

	if ( cMove.x || cMove.y )
		FrameMoved( cMove );
}



void BView::ViewScrolled( const BPoint& cDelta )
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::_ParentSized( const BPoint& cDelta )
{
	BRect cNewFrame = m->m_cFrame;

	if ( m->m_nResizeMask & B_FOLLOW_H_CENTER )
	{
		if ( m->m_nResizeMask & B_FOLLOW_LEFT )
		{
			cNewFrame.right = (cNewFrame.right * 2 + cDelta.x + 1) / 2;
		}

		if ( m->m_nResizeMask & B_FOLLOW_RIGHT )
		{
			cNewFrame.left = (cNewFrame.left * 2 + cDelta.x + 1) / 2;

			cNewFrame.right += cDelta.x;
		}
	}
	else
	{
		if ( m->m_nResizeMask & B_FOLLOW_RIGHT )
		{
			if ( (m->m_nResizeMask & B_FOLLOW_LEFT) == 0 )
			{
				cNewFrame.left += cDelta.x;
			}
			cNewFrame.right += cDelta.x;
		}
	}
	if ( m->m_nResizeMask & B_FOLLOW_BOTTOM )
	{
		if ( (m->m_nResizeMask & B_FOLLOW_TOP) == 0 )
		{
			cNewFrame.top += cDelta.y;
		}
		cNewFrame.bottom += cDelta.y;
	}

	SetFrame( cNewFrame );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::_BeginUpdate()
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		if ( server_token != -1 )
		{
			if ( m->m_nBeginPaintCount++ == 0 )
			{
				pcWindow->_AllocRenderCmd( DRC_BEGIN_UPDATE, this, sizeof(GRndHeader_s) );
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

void BView::_EndUpdate( void )
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		if ( --m->m_nBeginPaintCount == 0 )
		{
			pcWindow->_AllocRenderCmd( DRC_END_UPDATE, this, sizeof(GRndHeader_s) );
		}
	}
}



void BView::PushState()
{
	StateStruct* aState = (StateStruct*)malloc(sizeof(StateStruct));

	aState->origin = Origin();
	aState->scale = m->mScale;
	aState->drawingMode = DrawingMode();
	aState->penLocation = PenLocation();
	aState->highColor = HighColor();
	aState->lowColor = LowColor();
	aState->nextState = m->mStateStack;

	m->mStateStack = aState;
}

void BView::PopState()
{
	if (m->mStateStack != NULL)
	{
		StateStruct* aState = m->mStateStack;
		m->mStateStack = aState->nextState;

		SetOrigin(aState->origin);
		SetScale(aState->scale);
		SetDrawingMode(aState->drawingMode);
		SetHighColor(aState->highColor);
		SetLowColor(aState->lowColor);
		MovePenTo(aState->penLocation);

		free(aState);
	}
}

void BView::MovePenTo(BPoint pt)
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndSetPenPos_s* psCmd = static_cast<GRndSetPenPos_s*>(pcWindow->_AllocRenderCmd( DRC_SET_PEN_POS, this, sizeof( GRndSetPenPos_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->bRelative = false;
			psCmd->sPosition = pt;
		}
	}
}

void BView::MovePenTo(float x, float y)
{
	MovePenTo(BPoint(x, y));
}

void BView::MovePenBy( float x, float y )
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndSetPenPos_s* psCmd = static_cast<GRndSetPenPos_s*>(pcWindow->_AllocRenderCmd( DRC_SET_PEN_POS, this, sizeof( GRndSetPenPos_s )));
		if ( psCmd != NULL )
		{
			psCmd->bRelative = true;
			psCmd->sPosition = BPoint(x,y);
		}
	}
}

BPoint BView::PenLocation() const
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		return( pcWindow->_GetPenPosition( server_token ) );
	}

	return( BPoint( 0, 0 ) );
}

//---------------------------------------------------------------------------

void BView::SetPenSize(float size)
{
}

//---------------------------------------------------------------------------

float BView::PenSize() const
{
	return 1.0f;
}


void BView::_ReleaseFont()
{
	m->m_pcFont->Release();
}

/** Change the views text font.
 * \par Description:
 *        Replace the font used when rendering text and notify the view
 *        itself about the change by calling the BView::FontChanged()
 *        hook function.
 *
 *        Font's are reference counted and SetFont() will call
 *        Font::AddRef() on the new font and Font::Release() on the old
 *        font. This means that you retain ownership on the font even
 *        after BView::SetFont() returns. You must therefore normally call
 *        Font::Release() on the font after setting it.
 *
 *        The view will be affected by changes applied to the font after
 *        it is set. If someone change the properties of the font later
 *        the view will again be notified through the BView::FontChanged()
 *        hook
 *
 * \param pcFont
 *        Pointer to the new font. Its reference count will be
 *        increased by one.
 *
 * \sa DrawString(), BFont
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void BView::SetFont(BFont* font, uint32 mask)
{
	BWindow* pcWindow = Window();

	if ( font == m->m_pcFont )
	{
		return;
	}
	if ( m->m_pcFont != NULL )
	{
		_ReleaseFont();
	}
	if ( NULL != font )
	{
		m->m_pcFont = font;
		m->m_pcFont->AddRef();
	}
	else
	{
		m->m_pcFont = new BFont();
		if ( m->m_pcFont->SetFamilyAndStyle( SYS_FONT_FAMILY, SYS_FONT_PLAIN ) == 0 )
		{
			m->m_pcFont->SetProperties( 8.0f, 0.0f, 0.0f );
		}
	}

	if ( pcWindow != NULL )
	{
		GRndSetFont_s* psCmd = static_cast<GRndSetFont_s*>( pcWindow->_AllocRenderCmd( DRC_SET_FONT, this, sizeof( GRndSetFont_s ) ));

		if ( psCmd != NULL )
		{
			psCmd->hFontID = m->m_pcFont->GetFontID();
		}
	}
	FontChanged( m->m_pcFont );
}



BFont* BView::GetFont(void) const
{
	return( m->m_pcFont );
}


void BView::GetFont(BFont* font) const
{
	*font = *(m->m_pcFont);
}

void BView::TruncateString(BString* in_out,uint32 mode,float width) const
{
}


void BView::SetFontSize( float inSize )
{
	m->m_pcFont->SetSize(inSize);
}

//---------------------------------------------------------------------------


// Drawing Functions
//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap,	BRect srcRect, BRect dstRect)
{
	if ( !aBitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;
		
	DrawBitmap(aBitmap, srcRect, dstRect);
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap, BRect dstRect)
{
	if ( !aBitmap || !dstRect.IsValid())
		return;
	
	DrawBitmapAsync( aBitmap, aBitmap->Bounds(), dstRect);
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap)
{
	DrawBitmapAsync( aBitmap, PenLocation() );
}

//---------------------------------------------------------------------------

void BView::DrawBitmapAsync(const BBitmap* aBitmap, BPoint where)
{
	if ( !aBitmap )
		return;

	DrawBitmap(aBitmap, where);
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap)
{
	DrawBitmap( aBitmap, PenLocation() );	
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BPoint where)
{
	if ( !aBitmap )
		return;

	BRect r(aBitmap->fBounds);
	r.OffsetBy(where);
	DrawBitmap(aBitmap, r, r);
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BRect dstRect)
{
	if ( !aBitmap || !dstRect.IsValid())
		return;

	DrawBitmap( aBitmap, dstRect, dstRect);
}

//---------------------------------------------------------------------------

void BView::DrawBitmap(const BBitmap* aBitmap, BRect srcRect, BRect dstRect)
{
	if ( !aBitmap || !srcRect.IsValid() || !dstRect.IsValid())
		return;

	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndDrawBitmap_s* psCmd = static_cast<GRndDrawBitmap_s*>(pcWindow->_AllocRenderCmd( DRC_DRAW_BITMAP, this, sizeof( GRndDrawBitmap_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->hBitmapToken = aBitmap->fToken;
			psCmd->cDstRect     = dstRect;
			psCmd->cSrcRect     = srcRect;
		}
	}
}

//---------------------------------------------------------------------------

void BView::DrawChar(char aChar)
{
	char zStr[] = {aChar};
	DrawString( zStr, 1 );
}

//---------------------------------------------------------------------------

void BView::DrawChar(char aChar, const BPoint location)
{
	char		ch[2];
	ch[0]		= aChar;
	ch[1]		= '\0';

	DrawString( ch, 1, location );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, escapement_delta* delta)
{
	if ( !aString )
		return;

	DrawString( aString, -1 );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, BPoint location,
						escapement_delta* delta)
{
	if ( !aString )
		return;

	MovePenTo( location );
	DrawString( aString, strlen(aString) );
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, int32 length, escapement_delta* delta)
{
	if ( !aString )
		return;

	if ( length == -1 )
	{
		length = strlen( aString );
	}

	if ( length > 0 )
	{
		BWindow* pcWindow = Window();

		if ( pcWindow != NULL )
		{
			GRndDrawString_s* psCmd = static_cast<GRndDrawString_s*>( pcWindow->_AllocRenderCmd( DRC_DRAW_STRING, this,
																								sizeof( GRndDrawString_s ) + length - 1 ) );
			if ( psCmd != NULL )
			{
				psCmd->nLength = length;
				memcpy( psCmd->zString, aString, length );
			}
		}
	}
}

//---------------------------------------------------------------------------

void BView::DrawString(const char* aString, int32 length, BPoint location,
		escapement_delta* delta)
{
	if ( !aString )
		return;

	MovePenTo( location );
	DrawString( aString, length );
}

//---------------------------------------------------------------------------

void BView::StrokeEllipse(BPoint center, float xRadius, float yRadius,
		pattern p)
{
	BWindow*            pcWindow = Window();

	if ( pcWindow != NULL )
	{
		BRect r(center.x - xRadius, center.y - yRadius,
				center.x + xRadius, center.y + yRadius);
		
		GRndRect32_s* psCmd = static_cast<GRndRect32_s*>( pcWindow->_AllocRenderCmd( DRC_STROKE_ELLIPSE, this, sizeof( GRndRect32_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = r;
			psCmd->sColor = m->m_sFgColor;
		}
	}
}

//---------------------------------------------------------------------------

void BView::StrokeEllipse(BRect r, pattern p) 
{
	StrokeEllipse( 	r.LeftTop() + BPoint(r.Width()/2, r.Height()/2),
					r.Width()/2, r.Height()/2, p );
}

//---------------------------------------------------------------------------

void BView::FillEllipse(BPoint center, float xRadius, float yRadius,
		pattern p)
{
	BWindow*            pcWindow = Window();

	if ( pcWindow != NULL )
	{
		BRect r(center.x - xRadius, center.y - yRadius,
				center.x + xRadius, center.y + yRadius);
		
		GRndRect32_s* psCmd = static_cast<GRndRect32_s*>( pcWindow->_AllocRenderCmd( DRC_FILL_ELLIPSE, this, sizeof( GRndRect32_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = r;
			psCmd->sColor = m->m_sFgColor;
		}
	}
}

//---------------------------------------------------------------------------

void BView::FillEllipse(BRect r, pattern p) 
{
	FillEllipse( 	r.LeftTop() + BPoint(r.Width()/2, r.Height()/2),
					r.Width()/2, r.Height()/2, p );

}

//---------------------------------------------------------------------------

void BView::StrokeArc(BPoint center, float xRadius, float yRadius,
		float start_angle, float arc_angle, pattern p)
{
	StrokeArc( 	BRect(center.x-xRadius, center.y-yRadius, center.x+xRadius,
		center.y+yRadius), start_angle, arc_angle, p );
}

//---------------------------------------------------------------------------

void BView::StrokeArc(BRect r, float start_angle, float arc_angle,
		pattern p)
{
	BWindow*            pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndArc_s* psCmd = static_cast<GRndArc_s*>( pcWindow->_AllocRenderCmd( DRC_STROKE_ARC, this, sizeof( GRndArc_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = r;
			psCmd->sAngle = start_angle;
			psCmd->sSpan  = arc_angle;
			psCmd->sColor = m->m_sFgColor;
		}
	}
}

//---------------------------------------------------------------------------
									  
void BView::FillArc(BPoint center,float xRadius, float yRadius,
		float start_angle, float arc_angle,	pattern p)
{
	FillArc( 	BRect(center.x-xRadius, center.y-yRadius, center.x+xRadius,
		center.y+yRadius), start_angle, arc_angle, p );
}

//---------------------------------------------------------------------------

void BView::FillArc(BRect r, float start_angle, float arc_angle,
		pattern p)
{
	BWindow*            pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndArc_s* psCmd = static_cast<GRndArc_s*>( pcWindow->_AllocRenderCmd( DRC_FILL_ARC, this, sizeof( GRndArc_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = r;
			psCmd->sAngle = start_angle;
			psCmd->sSpan  = arc_angle;
			psCmd->sColor = m->m_sFgColor;
		}
	}
}

//---------------------------------------------------------------------------

void BView::StrokeBezier(BPoint* controlPoints, pattern p)
{
}

//---------------------------------------------------------------------------

void BView::FillBezier(BPoint* controlPoints, pattern p)
{
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPolygon* aPolygon,bool closed, pattern p)
{
	if(!aPolygon)
		return;
	
	StrokePolygon(aPolygon->fPts, aPolygon->fCount, aPolygon->Frame(), closed, p);
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPoint* ptArray, int32 numPts,bool closed, pattern p)
{
	if ( !ptArray )
		return;

	if (numPts > 1L)
	{
		BeginLineArray(numPts + (int)closed);

		for (int i=0; i < (numPts - 1); i++)
		{
			AddLine(ptArray[i], ptArray[i+1], HighColor());
		}

		if (closed)
			AddLine( ptArray[numPts - 1L], ptArray[0], HighColor() );

		EndLineArray();
	}
}

//---------------------------------------------------------------------------

void BView::StrokePolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
		bool closed, pattern p)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	pol.MapTo( pol.Frame(), bounds);
	StrokePolygon( &pol, closed, p );
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPolygon* aPolygon,pattern p)
{
	if ( !aPolygon )
		return;
		
	if ( aPolygon->fCount <= 2 )
		return;

// FIXME
}
//---------------------------------------------------------------------------

void BView::FillPolygon(const BPoint* ptArray, int32 numPts, pattern p)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	FillPolygon( &pol, p );
}

//---------------------------------------------------------------------------

void BView::FillPolygon(const BPoint* ptArray, int32 numPts, BRect bounds,
		pattern p)
{
	if ( !ptArray )
		return;

	BPolygon		pol( ptArray, numPts );
	pol.MapTo( pol.Frame(), bounds);
	FillPolygon( &pol, p );
}

//---------------------------------------------------------------------------

void BView::StrokeRect(BRect r, pattern p)
{
	BWindow*            pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndRect32_s* psCmd = static_cast<GRndRect32_s*>( pcWindow->_AllocRenderCmd( DRC_STROKE_RECT32, this, sizeof( GRndRect32_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = r;
			psCmd->sColor = m->m_sFgColor;
		}
	}
}

//---------------------------------------------------------------------------

void BView::FillRect(BRect r, pattern p)
{
	BWindow*            pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndRect32_s* psCmd = static_cast<GRndRect32_s*>( pcWindow->_AllocRenderCmd( DRC_FILL_RECT32, this, sizeof( GRndRect32_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = r;
			psCmd->sColor = m->m_sFgColor;
		}
	}
}


void BView::FillRect( BRect cRect, rgb_color sColor )
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndRect32_s* psCmd = static_cast<GRndRect32_s*>(pcWindow->_AllocRenderCmd( DRC_FILL_RECT32, this, sizeof( GRndRect32_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = cRect;
			psCmd->sColor = sColor;
		}
	}
}

//---------------------------------------------------------------------------

void BView::StrokeRoundRect(BRect r, float xRadius, float yRadius,
		pattern p)
{
}

//---------------------------------------------------------------------------

void BView::FillRoundRect(BRect r, float xRadius, float yRadius,
		pattern p)
{
}

//---------------------------------------------------------------------------

void BView::FillRegion(BRegion* a_region, pattern p)
{
	if ( !a_region )
		return;
	
	// TODO: this should be done on the server side
	for(int32 i=0; i < a_region->CountRects(); i++)
		FillRect(a_region->RectAt(i), p);

}

//---------------------------------------------------------------------------

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
		BRect bounds, pattern p)
{
	BeginLineArray(3);

	AddLine( pt1, pt2, HighColor() );
	AddLine( pt2, pt3, HighColor() );
	AddLine( pt3, pt1, HighColor() );

	EndLineArray();
}

//---------------------------------------------------------------------------

void BView::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect		bounds(pt1, pt1);
		
		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;

		if (pt2.y < bounds.top)
			bounds.top = pt2.y;

		if (pt2.x > bounds.right)
			bounds.right = pt2.x;

		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;
			
		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;

		if (pt3.y < bounds.top)
			bounds.top = pt3.y;

		if (pt3.x > bounds.right)
			bounds.right = pt3.x;

		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		

		StrokeTriangle( pt1, pt2, pt3, bounds, p );
}

//---------------------------------------------------------------------------

void BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3, pattern p)
{
		// we construct the smallest rectangle that contains the 3 points
		// for the 1st point
		BRect		bounds(pt1, pt1);

		// for the 2nd point		
		if (pt2.x < bounds.left)
			bounds.left = pt2.x;

		if (pt2.y < bounds.top)
			bounds.top = pt2.y;

		if (pt2.x > bounds.right)
			bounds.right = pt2.x;

		if (pt2.y > bounds.bottom)
			bounds.bottom = pt2.y;
			
		// for the 3rd point
		if (pt3.x < bounds.left)
			bounds.left = pt3.x;

		if (pt3.y < bounds.top)
			bounds.top = pt3.y;

		if (pt3.x > bounds.right)
			bounds.right = pt3.x;

		if (pt3.y > bounds.bottom)
			bounds.bottom = pt3.y; 		

		FillTriangle( pt1, pt2, pt3, bounds, p );
}

//---------------------------------------------------------------------------

void BView::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3,
		BRect bounds, pattern p)
{
	// TODO: send to server (and fill, not stroke!)
	StrokeTriangle(pt1, pt2, pt3, bounds);
}

//---------------------------------------------------------------------------

void BView::StrokeLine(BPoint toPt, pattern p)
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndLine32_s* psCmd = static_cast<GRndLine32_s*>(pcWindow->_AllocRenderCmd( DRC_LINE32, this, sizeof( GRndLine32_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sToPos = toPt;
		}
	}
}

//---------------------------------------------------------------------------

void BView::StrokeLine(BPoint pt0, BPoint pt1, pattern p){
	BWindow* pcWindow = Window();

	MovePenTo(pt0);

	if ( pcWindow != NULL )
	{
		GRndLine32_s* psCmd = static_cast<GRndLine32_s*>( pcWindow->_AllocRenderCmd( DRC_LINE32, this, sizeof( GRndLine32_s )) );
		if ( psCmd != NULL )
		{
			psCmd->sToPos = pt1;
		}
	}
}

//---------------------------------------------------------------------------

void BView::StrokeShape(BShape* shape, pattern p){
	if ( !shape )
		return;
}

//---------------------------------------------------------------------------

void BView::FillShape(BShape* shape, pattern p){
	if ( !shape )
		return;
}

//---------------------------------------------------------------------------

void BView::BeginLineArray(int32 count)
{
	if (m->mLineArrayAllocation > 0)
		free(m->mLineArray);

	m->mLineArray = (LineArrayElement*)malloc(sizeof(LineArrayElement) * count);
	m->mLineArrayAllocation = count;
	m->mLineArrayCount = 0;
}

//---------------------------------------------------------------------------

void BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	if (m->mLineArrayCount < m->mLineArrayAllocation)
	{
		m->mLineArray[m->mLineArrayCount].from = pt0;
		m->mLineArray[m->mLineArrayCount].to = pt1;
		m->mLineArray[m->mLineArrayCount].color = col;
		m->mLineArrayCount++;
	}
}

//---------------------------------------------------------------------------

void BView::EndLineArray()
{
	int x;
	
	if (m->mLineArrayAllocation > 0)
	{
		for (x=0; x < m->mLineArrayCount; x++)
		{
			// FIXME: This should obviously send the line array to the appserver
			// instead of the cop-out you see below.
			SetHighColor(m->mLineArray[x].color);
			StrokeLine(m->mLineArray[x].from, m->mLineArray[x].to);
		}

		m->mLineArrayAllocation = 0;
		m->mLineArrayCount = 0;
		free(m->mLineArray);
	}
}

//---------------------------------------------------------------------------

void BView::EraseRect( const BRect& cRect )
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndRect32_s* psCmd = static_cast<GRndRect32_s*>(pcWindow->_AllocRenderCmd( DRC_FILL_RECT32, this, sizeof( GRndRect32_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->sRect  = cRect;
			psCmd->sColor = m->m_sEraseColor;
		}
	}
}




//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::DrawFrame( const BRect& a_cRect, uint32 nStyle )
{
	BRect cRect(a_cRect);
	cRect.Set(floor(cRect.left), floor(cRect.top),
			  floor(cRect.right), floor(cRect.bottom));
	bool bSunken = false;

	if ( ((nStyle & FRAME_RAISED) == 0) && (nStyle & (FRAME_RECESSED)) )
	{
		bSunken = true;
	}

	rgb_color sFgCol = get_default_color( COL_SHINE );
	rgb_color sBgCol = get_default_color( COL_SHADOW );

	if ( nStyle & FRAME_DISABLED )
	{
		sFgCol = Tint( sFgCol, 0.6f );
		sBgCol = Tint( sBgCol, 0.4f );
	}
	rgb_color sFgShadowCol = Tint( sFgCol, 0.6f );
	rgb_color sBgShadowCol = Tint( sBgCol, 0.5f );

	if ( nStyle & FRAME_FLAT )
	{
		SetHighColor( (bSunken) ? sBgCol : sFgCol );
		MovePenTo( cRect.left, cRect.bottom );
		StrokeLine( BPoint( cRect.left, cRect.top ) );
		StrokeLine( BPoint( cRect.right, cRect.top ) );
		StrokeLine( BPoint( cRect.right, cRect.bottom ) );
		StrokeLine( BPoint( cRect.left, cRect.bottom ) );
	}
	else
	{
		if ( nStyle & FRAME_THIN )
		{
			SetHighColor( (bSunken) ? sBgCol : sFgCol );
		}
		else
		{
			SetHighColor( (bSunken) ? sBgCol : sFgShadowCol );
		}

		MovePenTo( cRect.left, cRect.bottom );
		StrokeLine( BPoint( cRect.left, cRect.top ) );
		StrokeLine( BPoint( cRect.right, cRect.top ) );

		if ( nStyle & FRAME_THIN )
		{
			SetHighColor( (bSunken) ? sFgCol : sBgCol );
		}
		else
		{
			SetHighColor( (bSunken) ? sFgCol : sBgShadowCol );
		}
		StrokeLine( BPoint( cRect.right, cRect.bottom ) );
		StrokeLine( BPoint( cRect.left, cRect.bottom ) );


		if ( (nStyle & FRAME_THIN) == 0 )
		{
			if ( nStyle & FRAME_ETCHED )
			{
				SetHighColor( (bSunken) ? sBgCol : sFgCol );

				MovePenTo( cRect.left + 1.0f, cRect.bottom - 1.0f );

				StrokeLine( BPoint( cRect.left + 1.0f, cRect.top + 1.0f ) );
				StrokeLine( BPoint( cRect.right - 1.0f, cRect.top + 1.0f ) );

				SetHighColor( (bSunken) ? sFgCol : sBgCol );

				StrokeLine( BPoint( cRect.right - 1.0f, cRect.bottom - 1.0f ) );
				StrokeLine( BPoint( cRect.left + 1.0f, cRect.bottom - 1.0f ) );
			}
			else
			{
				SetHighColor( (bSunken) ? sBgShadowCol : sFgCol );

				MovePenTo( cRect.left + 1.0f, cRect.bottom - 1.0f );

				StrokeLine( BPoint( cRect.left + 1.0f, cRect.top + 1.0f ) );
				StrokeLine( BPoint( cRect.right - 1.0f, cRect.top + 1.0f ) );

				SetHighColor( (bSunken) ? sFgShadowCol : sBgCol );

				StrokeLine( BPoint( cRect.right - 1.0f, cRect.bottom - 1.0f ) );
				StrokeLine( BPoint( cRect.left + 1.0f, cRect.bottom - 1.0f ) );
			}
			if ( (nStyle & FRAME_TRANSPARENT) == 0 )
			{
				EraseRect( BRect( cRect.left + 2.0f, cRect.top + 2.0f, cRect.right - 2.0f, cRect.bottom - 2.0f) );
			}
		}
		else
		{
			if ( (nStyle & FRAME_TRANSPARENT) == 0 )
			{
				EraseRect( BRect( cRect.left + 1.0f, cRect.top + 1.0f, cRect.right - 1.0f, cRect.bottom - 1.0f) );
			}
		}
	}
}


/** Copy a rectangle from one location to another within the view.
 * \par Description:
 *        CopyBits() will copy the source rectangle to the destination
 *        rectangle using the blitter. If parts of the source rectangle
 *        is obscured (by another window/view or the screen edge) and
 *        the same area in the destination area is not that area in the
 *        destination rectangle will be invalidated (see Invalidate()).
 *
 * \par Note:
 *        Scrolling an area within the view might affect the current
 *        "damage-list" of the view and cause a repaint message to be
 *        sent to the view. If CopyBits() is called repeatedly and
 *        the application for some reason can't handle the generated
 *        repaint messages fast enough more and more damage-rectangles
 *        will accumulate in the view's damage list slowing things
 *        further down until the appserver is brought to a grinding halt.
 *        To avoid this situation the next draw message received after
 *        a call to ScrollRect() will cause Sync() instead of Flush() to
 *        be called when Draw() returns to syncronize the application
 *        with the appserver. So even though the ScrollRect() member
 *        itself is asyncronous it might cause Sync() to be called and
 *        will have the performance implications mentioned in the
 *        documentation of Sync()
 * 
 * \param src
 *        The source rectangle in the views coordinate system.
 * \param dst
 *        The destination rectangle in the views coordinate system.
 *        This rectangle should have the same size but a difference
 *        position than the \p cSrcRect. In a future version it might be
 *        possible to scale the rectangle by using a different size so
 *        make sure they don't differ or you might get a surprice
 *        some day.
 *
 * \sa ScrollTo(), ScrollBy(), Sync()
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

//---------------------------------------------------------------------------

void BView::CopyBits(BRect src, BRect dst)
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndCopyRect_s* psCmd = static_cast<GRndCopyRect_s*>(pcWindow->_AllocRenderCmd( DRC_COPY_RECT, this, sizeof( GRndCopyRect_s ) ));
		if ( psCmd != NULL )
		{
			psCmd->cSrcRect = src;
			psCmd->cDstRect = dst;
		}
	}
}

//---------------------------------------------------------------------------

/** Return the width of the given string using the current view font
 * \author        Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

float BView::StringWidth( const char* pzString) const
{
	if ( m->m_pcFont == NULL )
	{
		printf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return( 0 );
	}
	return( m->m_pcFont->StringWidth( pzString ) );
}

float BView::StringWidth( const char* pzString, int32 nLength ) const
{
	if ( m->m_pcFont == NULL )
	{
		printf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return( 0 );
	}
	return( m->m_pcFont->StringWidth( pzString, nLength ) );
}


/** Return the number of characters in pzString which will fit entirely in vWidth
 * \author        Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

int BView::GetStringLength( const char* pzString, float vWidth, bool bIncludeLast ) const
{
	if ( m->m_pcFont == NULL )
	{
		printf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return( 0 );
	}
	return( m->m_pcFont->GetStringLength( pzString, vWidth, bIncludeLast ) );
}


/** Return the number of characters in pzString which will fit entirely in vWidth
 * \author        Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

int BView::GetStringLength( const char* pzString, int nLength, float vWidth, bool bIncludeLast ) const
{
	if ( m->m_pcFont == NULL )
	{
		printf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return( 0 );
	}

	return( m->m_pcFont->GetStringLength( pzString, nLength, vWidth, bIncludeLast ) );
}


/** Return the number of characters in pzString which will fit entirely in vWidth
 * \author        Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

int BView::GetStringLength( const std::string& cString, float vWidth, bool bIncludeLast ) const
{
	if ( m->m_pcFont == NULL )
	{
		printf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return( 0 );
	}
	return( m->m_pcFont->GetStringLength( cString, vWidth, bIncludeLast ) );
}

/** For each string in apzStringArray, return the number of characters which
 *  will fit entirely in vWidth
 * \author        Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

void BView::GetStringLengths( const char** apzStringArray, const int* anLengthArray, int nStringCount,
                             float vWidth, int* anMaxLengthArray, bool bIncludeLast ) const
{
	if ( m->m_pcFont == NULL )
	{
		printf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return;
	}
	m->m_pcFont->GetStringLengths( apzStringArray, anLengthArray, nStringCount, vWidth,
								anMaxLengthArray, bIncludeLast );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::GetFontHeight( font_height* psHeight ) const
{
	if ( m->m_pcFont == NULL )
	{
		printf( "Warning: %s() View %s has no font\n", __FUNCTION__, m->m_cTitle.c_str() );
		return;
	}
	m->m_pcFont->GetHeight( psHeight );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BPoint BView::ConvertToWindow( BPoint cPoint ) const
{
	if ( parent != NULL )
		return( parent->ConvertToWindow( cPoint + LeftTop() + m->m_cScrollOffset ) );

	return( cPoint );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::ConvertToWindow( BPoint* pcPoint ) const
{
	if ( parent != NULL )
	{
		*pcPoint += LeftTop() + m->m_cScrollOffset;
		parent->ConvertToWindow( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BRect BView::ConvertToWindow( BRect cRect ) const
{
	if ( parent != NULL )
		return( parent->ConvertToWindow( cRect.OffsetBySelf(LeftTop() + m->m_cScrollOffset) ) );

	return( cRect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::ConvertToWindow( BRect* pcRect ) const
{
	if ( parent != NULL )
	{
		pcRect->OffsetBy(LeftTop() + m->m_cScrollOffset);
		parent->ConvertToWindow( pcRect );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BPoint BView::ConvertFromWindow( BPoint cPoint ) const
{
	if ( parent != NULL )
		return( parent->ConvertFromWindow( cPoint - LeftTop() - m->m_cScrollOffset ) );

	return( cPoint );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::ConvertFromWindow( BPoint* pcPoint ) const
{
	if ( parent != NULL )
	{
		*pcPoint -= LeftTop() + m->m_cScrollOffset;
		parent->ConvertFromWindow( pcPoint );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BRect BView::ConvertFromWindow( BRect cRect ) const
{
	if ( parent != NULL )
		return( parent->ConvertFromWindow( cRect.OffsetBySelf(- LeftTop() - m->m_cScrollOffset) ) );

	return( cRect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::ConvertFromWindow( BRect* pcRect ) const
{
	if ( parent != NULL )
	{
		pcRect->OffsetBy(-m->m_cFrame.left, -m->m_cFrame.top);
		pcRect->OffsetBy(-m->m_cScrollOffset);
		parent->ConvertFromWindow( pcRect );
	}
}

//---------------------------------------------------------------------------

void BView::InvertRect(BRect r)
{
	PushState();

	SetDrawingMode(B_OP_INVERT);
	SetHighColor(0, 0, 0);
	FillRect(r);

	PopState();
}

// View Hierarchy Functions
//---------------------------------------------------------------------------
void BView::AddChild(BView* child, BView* before)
{
	// FIXME: before is ignored

	_LinkChild( child, true );

	if ( m->m_nFlags & B_NAVIGABLE )
	{
		child->SetTabOrder( s_nNextTabOrder++ );
	}

	child->_Attached( Window(), this, -1, fShowLevel );
}

//---------------------------------------------------------------------------

bool BView::RemoveChild(BView* child)
{
	BWindow* pcWindow = Window();

	_UnlinkChild( child );

	if ( NULL != pcWindow && -1 != server_token )
	{
		child->_Detached( true, fShowLevel );
		pcWindow->Sync();
	}

	return true;
}

//---------------------------------------------------------------------------

int32 BView::CountChildren() const
{
	uint32		noOfChildren 	= 0;
	BView		*aChild			= first_child;
	
	while ( aChild != NULL )
	{
		noOfChildren++;
		aChild		= aChild->next_sibling;
	}

	return noOfChildren;
}

//---------------------------------------------------------------------------

BView* BView::ChildAt(int32 index) const
{
	BView		*child;

	if ( index >= 0 )
	{
		for (child = first_child ; NULL != child ; child = child->next_sibling )
		{
			if ( 0 == index )
			{
				return child;
			}
			index--;
		}
	}
	return NULL;
}

BView* BView::ChildAt( const BPoint& cPos ) const
{
	BView* child;

	for ( child = first_child ; NULL != child ; child = child->next_sibling )
	{
		if ( child->fShowLevel > 0 )
		{
			continue;
		}

		if ( !( (cPos.x < child->m->m_cFrame.left)  ||
				(cPos.x > child->m->m_cFrame.right) ||
				(cPos.y < child->m->m_cFrame.top)   ||
				(cPos.y > child->m->m_cFrame.bottom)) )
		{
			return child;
		}
	}
	return NULL;
}


//---------------------------------------------------------------------------

BView* BView::NextSibling() const
{
	return next_sibling;
}

//---------------------------------------------------------------------------

BView* BView::PreviousSibling() const
{
	return prev_sibling;	
}

//---------------------------------------------------------------------------

bool BView::RemoveSelf()
{
	if(!parent)
		return false;

	return parent->RemoveChild(this);
}

//---------------------------------------------------------------------------

BView* BView::Parent() const
{
	return parent;
}

//---------------------------------------------------------------------------

BView* BView::FindView(const char* name) const
{
    BView* child, *pcView;

    for (child = first_child ; NULL != child ; child = child->next_sibling )
    {
		if ( strcmp(name, child->m->m_cTitle.c_str()) == 0 )
		{
			return child;
		}
		
	    	pcView = child->FindView(name);
		if ( pcView != NULL )
		{
			return( pcView );
		}
    }
    return NULL;
}

//---------------------------------------------------------------------------

BRect BView::GetNormalizedBounds() const
{
	return( m->m_cFrame.OffsetToCopy(0,0) );
}


void BView::_ConstrictRectangle( BRect* pcRect, const BPoint& cOffset )
{
	BPoint cOff = cOffset - BPoint( m->m_cFrame.left, m->m_cFrame.top ) - m->m_cScrollOffset;
	*pcRect = *pcRect & m->m_cFrame.OffsetByCopy(cOff);
	if ( parent != NULL )
	{
		parent->_ConstrictRectangle( pcRect, cOff );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
float BView::Width() const
{
	return( m->m_cFrame.Width() );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

const char* BView::Title() const
{
	return( m->m_cTitle.c_str() );
}


/** Called to notify the view that the font has changed
 * \par Description:
 *        FontChanged() is a virtual hook function that can be
 *        overloaded by inheriting classes to track changes to the
 *        view's font.
 *
 *        This hook function is called whenver the font is replaced
 *        through the BView::SetFont() member or if the currently
 *        assigned font is modified in a way that whould alter the
 *        graphical appearance of the font.
 *
 * \par Note:
 *        BView::SetFont() will call FontChanged() syncronously and will
 *        cause FontChanged() to be called even if the view is not yet
 *        added to a window. Changes done to the font-object cause a
 *        message to be sent to the window thread and FontChanged()
 *        will then be called asyncronously from the window thread when
 *        the message arrive. For this reason it is only possible to
 *        track changes done to the font object itself when the view is
 *        added to a window.
 *        
 * \param pcNewFont
 *        Pointer to the affected font (same as returned by GetFont()).
 *
 * \sa SetFont(), Font
 * \author        Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void BView::FontChanged( BFont* pcNewFont )
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BView::Ping( int nSize )
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		pcWindow->_AllocRenderCmd( DRC_PING, this, sizeof( GRndHeader_s ) + nSize );
	}
}


int BView::_GetHandle() const
{
	return( server_token );
}


void BView::_SetMouseMoveRun( int nValue )
{
    m->m_nMouseMoveRun = nValue;
}


int BView::_GetMouseMoveRun() const
{
	return( m->m_nMouseMoveRun );
}


void BView::_SetMouseMode( int nMode )
{
	m->m_nMouseMode = nMode;
}


int BView::_GetMouseMode() const
{
	return( m->m_nMouseMode );
}


void BView::_SetHScrollBar( BScrollBar* pcScrollBar )
{
	fHorScroller = pcScrollBar;
}


void BView::_SetVScrollBar( BScrollBar* pcScrollBar )
{
	fVerScroller = pcScrollBar;
}

/** Add a rectangle to the damage list.
 * \par Description:
 *        To avoid rendering the whole view when a new area is made
 *        visible, the appserver maintain a damage list containing
 *        areas made visible since the last draw message was sent
 *        to the view. When the View respond to the draw message
 *        the appserver will clip the rendering to the damage list
 *        to avoid rendering any pixels that are still intact.
 *        By calling Invalidate() you can force an area
 *        into the damage list, and make the appserver send a
 *        draw message to the view. This will again make the
 *        Draw() member be called to revalidate the damaged
 *        areas.
 * \param cRect
 *        The rectangle to invalidate.
 * \param bRecurse
 *        If true cRect will also be converted into each children's
 *        coordinate system and added to their damage list.
 *
 * \sa Invalidate( bool ), Draw()
 * \author        Kurt Skauen (kurt.skauen@c2i.net)
 *****************************************************************************/

void BView::Invalidate(BRect invalRect, bool bRecurse)
{
	BWindow* pcWnd = Window();

	if ( pcWnd == NULL )
	{
		return;
	}

	GRndInvalidateRect_s* psCmd = static_cast<GRndInvalidateRect_s*>(pcWnd->_AllocRenderCmd( DRC_INVALIDATE_RECT, this, sizeof(GRndInvalidateRect_s) ));

	if ( psCmd != NULL )
	{
		psCmd->m_cRect    = invalRect;
		psCmd->m_bRecurse = bRecurse;
		Flush();
	}
}

/** Invalidate the whole view.
 * \par Description:
 *        Same as calling Invalidate( Bounds() ), but more efficient since
 *        the appserver know that the whole view is to be invalidated, and
 *        does not have to merge the rectangle into the clipping region.
 * \param bRecurse - True if all children should be invalidated recursivly as well.
 * \sa Invalidate( BRect, bool ), Draw()
 * \author        Kurt Skauen (kurt.skauen@c2i.net)
 *****************************************************************************/

void BView::Invalidate(bool bRecurse)
{
	BWindow* pcWnd = Window();
	if ( pcWnd == NULL )
	{
		return;
	}

	GRndInvalidateView_s* psCmd = static_cast<GRndInvalidateView_s*>(pcWnd->_AllocRenderCmd( DRC_INVALIDATE_VIEW, this, sizeof(GRndInvalidateView_s) ));

	if ( psCmd != NULL )
	{
		psCmd->m_bRecurse = bRecurse;
		Flush();
	}
}

status_t BView::SetEventMask(uint32 mask, uint32 options)
{
	fEventMask = mask;
	return B_OK;
}


uint32 BView::EventMask()
{
	return fEventMask;
}


status_t BView::SetMouseEventMask(uint32 mask, uint32 options)
{
	return B_OK;
}


/** Show/hide a view and all its children.
 * \par Description:
 *        Calling show with bVisible == false on a view have the same effect as
 *        unlinking it from it's parent, except that the view will remember it's
 *        entire state like the font, colors, position, depth, etc etc. A subsequent
 *        call to Show() with bVisible == true will restore the view compleatly.
 *
 *        Calls to Show() can be nested. If it is called twice with bVisible == false
 *        it must be called twice with bVisible == true before the view is made
 *        visible. It is safe to call Show(false) on a view before it is attached to a
 *        window. The view will then remember it's state and will not be visible
 *        when attached to a window later on.
 *
 *        A hidden view will not receive draw messages or input events from keybord
 *        or mouse. It will still receive messages targeted directly at the view.
 * \par Note:
 *        It is NOT legal to show a visible view! Calling Show(true) before
 *        Show(false) is an error.
 * \param
 *        bVisible - A boolean telling if the view should be hidden or viewed.
 * \sa IsVisible(), Window::Show()
 * \author        Kurt Skauen (kurt.skauen@c2i.net)
 *//////////////////////////////////////////////////////////////////////////////



void BView::show_view( bool bVisible )
{
	BWindow* pcWindow = Window();

	if ( pcWindow != NULL )
	{
		GRndShowView_s* psCmd;
		psCmd = (GRndShowView_s*) pcWindow->_AllocRenderCmd( DRC_SHOW_VIEW, this, sizeof(GRndShowView_s) );
		if ( psCmd != NULL )
		{
			psCmd->bVisible = bVisible;
		}
	}
	
	for ( BView* pcChild = m->m_pcBottomChild ; pcChild != NULL ; pcChild = pcChild->prev_sibling )
	{
		pcChild->_IncHideCount( bVisible );
	}
	Flush();
}

void BView::MoveBy( BPoint cDelta )
{
	MoveBy(cDelta.x, cDelta.y);
}

void BView::MoveBy(float dh, float dv)
{
	BRect	r(m->m_cFrame);
	r.OffsetBy(dh, dv);
	SetFrame(r);
}

void BView::MoveTo(BPoint where)
{
	MoveTo(where.x, where.y);
}


void BView::MoveTo( float x, float y )
{
	BRect	r(m->m_cFrame);
	r.OffsetBy(-LeftTop());
	r.OffsetBy(x, y);
	SetFrame(r);
}


void BView::ResizeBy( const BPoint& cDelta )
{
	SetFrame( BRect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.right + cDelta.x, m->m_cFrame.bottom + cDelta.y ) );
}

void BView::ResizeTo(const BPoint& cSize)
{
	SetFrame( BRect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.left + cSize.x, m->m_cFrame.top + cSize.y ) );
}

void BView::ResizeBy(float dh, float dv)
{
	SetFrame( BRect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.right + dh, m->m_cFrame.bottom + dv ) );
}

void BView::ResizeTo(float width, float height)
{
	SetFrame( BRect( m->m_cFrame.left, m->m_cFrame.top, m->m_cFrame.left + width, m->m_cFrame.top + height ) );
}

// Note: A positive x value will scroll this view left, and a positive
// y value will scroll this view up.
void BView::ScrollBy(float dh, float dv)
{
	// Need to negate values so that server scrolls in the correct direction
	// and m_cScrollOffset is adjusted to the correct value. Therefore,
	// positive values given to ScrollBy() will become negative at this point.
	dh = -dh;
	dv = -dv;
	BPoint cDelta(dh, dv);

	if (dh || dv)
	{
		// for m_cScrollOffset, negative x shifts view to the left,
		// and negative y shifts view up
		m->m_cScrollOffset += cDelta;
		BWindow* pcWindow = Window();

		if ( pcWindow != NULL )
		{
			GRndScrollView_s* psCmd = (GRndScrollView_s*)pcWindow->_AllocRenderCmd( DRC_SCROLL_VIEW, this,
																				sizeof( GRndScrollView_s ) );
			if ( psCmd != NULL )
			{
				// negative x scrolls left, and negative y scrolls up
				psCmd->cDelta = cDelta;
			}
		}

		if ( dh != 0 && fHorScroller != NULL )
		{
			fHorScroller->SetValue( -m->m_cScrollOffset.x );
		}

		if ( dv != 0 && fVerScroller != NULL )
		{
			fVerScroller->SetValue( -m->m_cScrollOffset.y );
		}
		ViewScrolled( cDelta );
	}
}


// Scrolls this view according to the given horizontal and vertical
// scrollbar positions indicated by cTopLeft.
void BView::ScrollTo(BPoint where)
{
	// New position of this view should be the opposite
	// of cTopLeft because this view should scroll in opposite
	// direction of scroll thumb movement.
	where = -where;

	// subtract cTopLeft from m->m_cScrollOffset to get
	// positive x for scrolling view left, and positive y for
	// scrolling view up.
	BPoint pt(m->m_cScrollOffset - where);
	ScrollBy(pt.x, pt.y);
}

void BView::MakeFocus(bool focusState)
{
}

BScrollBar* BView::ScrollBar(orientation posture) const
{
	BScrollBar* aScrollBar = NULL;

	switch (posture)
	{
		case B_HORIZONTAL:
			aScrollBar = fHorScroller;

		case B_VERTICAL:
			aScrollBar = fVerScroller;
	}

	return aScrollBar;
}

BHandler *BView::ResolveSpecifier(BMessage *msg,int32 index,BMessage *specifier,int32 form,const char *property)
{
	return NULL;
}

status_t BView::GetSupportedSuites(BMessage *data)
{
	return B_ERROR;
}

void BView::SetScale(float scale) const
{
	m->mScale = scale;
}

status_t BView::Perform(perform_code d, void *arg)
{
	return B_ERROR;
}

//---------------------------------------------------------------------------

void BView::_ReservedView2(){}
void BView::_ReservedView3(){}
void BView::_ReservedView4(){}
void BView::_ReservedView5(){}
void BView::_ReservedView6(){}
void BView::_ReservedView7(){}
void BView::_ReservedView8(){}

#if !_PR3_COMPATIBLE_
void BView::_ReservedView9(){}
void BView::_ReservedView10(){}
void BView::_ReservedView11(){}
void BView::_ReservedView12(){}
void BView::_ReservedView13(){}
void BView::_ReservedView14(){}
void BView::_ReservedView15(){}
void BView::_ReservedView16(){}
#endif


//---------------------------------------------------------------------------

#if 0

// Now defined in Message.h since they are needed elsewhere

inline rgb_color _get_rgb_color( uint32 color )
{
	rgb_color		c;
	c.red			= (color & 0xFF000000) >> 24;
	c.green			= (color & 0x00FF0000) >> 16;
	c.blue			= (color & 0x0000FF00) >> 8;
	c.alpha			= (color & 0x000000FF);

	return c;
}

//---------------------------------------------------------------------------

inline uint32 _get_uint32_color( rgb_color c )
{
	uint32			color;
	color			= (c.red << 24) +
					  (c.green << 16) +
					  (c.blue << 8) +
					  c.alpha;
	return color;
}
#endif

//---------------------------------------------------------------------------

inline rgb_color _set_static_rgb_color( uint8 r, uint8 g, uint8 b, uint8 a )
{
	rgb_color		color;
	color.red		= r;
	color.green		= g;
	color.blue		= b;
	color.alpha		= a;

	return color;
}

//---------------------------------------------------------------------------

inline void _set_ptr_rgb_color( rgb_color* c, uint8 r, uint8 g,uint8 b, uint8 a )
{
	c->red			= r;
	c->green		= g;
	c->blue			= b;
	c->alpha		= a;
}

//---------------------------------------------------------------------------

inline bool _rgb_color_are_equal( rgb_color c1, rgb_color c2 )
{
	return _get_uint32_color( c1 ) == _get_uint32_color( c2 );
}

//---------------------------------------------------------------------------

inline bool _is_new_pattern( const pattern& p1, const pattern& p2 )
{
	if ( memcmp( &p1, &p2, sizeof(pattern) ) == 0 )
		return false;
	else
		return true;
}

// Attaches this BView and all of its descendants to pcWindow's (which, if non-NULL, is the BWindow
// containing pcParent) and pcParent's hierarchy.
// Then, this method notifies this BView and all of descendants with AttachedToWindow() and AllAttached().
void BView::_Attached( BWindow* pcWindow, BView* pcParent, int hHandle, int nHideCount )
{	
	// This method will make sure that this BView and all of its
	// descendants are attached to the given window.
	_AttachedTree( pcWindow, pcParent, hHandle, nHideCount );
	
	if ( pcWindow != NULL )
	{
		_NotifyAttachedToWindow();
		_NotifyAllAttached();
	}
	
}

// Attaches this BView and all of its descendants to pcWindow's (which, if non-NULL, is the BWindow
// containing pcParent) and pcParent's hierarchy.
void BView::_AttachedTree( BWindow* pcWindow, BView* pcParent, int hHandle, int nHideCount )
{
	fShowLevel += nHideCount;
	if ( pcWindow != NULL )
	{
		pcWindow->Flush();
		pcWindow->AddHandler( this );

		if ( hHandle != -1 )
		{
			server_token = hHandle;
		}
		else
		{
			BMessage cReq( AS_LAYER_CREATE );

			cReq.AddInt32( "top_view", pcWindow->_GetTopView()->server_token );
			cReq.AddInt32( "flags", m->m_nFlags );
			cReq.AddPointer( "user_object", this );
			cReq.AddString( "name", Name() );
			cReq.AddRect( "frame", Frame() );
			cReq.AddInt32( "parent_view", (pcParent) ? pcParent->server_token : -1 );
			cReq.AddInt32( "fg_color", _get_uint32_color(m->m_sFgColor) );
			cReq.AddInt32( "bg_color", _get_uint32_color(m->m_sBgColor) );
			cReq.AddInt32( "er_color", _get_uint32_color(m->m_sEraseColor) );
			cReq.AddInt32( "hide_count", (hHandle==-1) ? fShowLevel : 0 );
			cReq.AddPoint( "scroll_offset", m->m_cScrollOffset );

			BMessage cReply;

			BMessenger( pcWindow->_GetAppserverPort() ).SendMessage( &cReq, &cReply );

			cReply.FindInt32( "handle", &server_token );
		}
		SetNextHandler( pcParent );

		BFont* pcFont;

		if ( NULL != m->m_pcFont )
		{
			GRndSetFont_s* psCmd = static_cast<GRndSetFont_s*>( pcWindow->_AllocRenderCmd( DRC_SET_FONT, this, sizeof( GRndSetFont_s ) ));
			if ( psCmd != NULL )
			{
				psCmd->hFontID = m->m_pcFont->GetFontID();
			}
		}
		else
		{
			pcFont = new BFont();
			pcFont->SetFamilyAndStyle( SYS_FONT_FAMILY, SYS_FONT_PLAIN );
			pcFont->SetProperties( 8.0f, 0.0f, 0.0f );
			SetFont( pcFont );
			pcFont->Release();
		}

		Flush();
	}

	// add all descendant views to hierarchy
	for ( BView* pcChild = m->m_pcBottomChild ; NULL != pcChild ; pcChild = pcChild->prev_sibling )
	{
		pcChild->_AttachedTree( pcWindow, this, -1, nHideCount );
	}
}

//---------------------------------------------------------------------------

/* TODO:
 	-implement SetDiskMode(). What's with this method? What does it do? test!
 		does it has something to do with DrawPictureAsync( filename* .. )?
	-implement DrawAfterChildren()
*/
