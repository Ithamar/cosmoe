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
//	File Name:		View.h
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BView is the base class for all views (clipped regions
//					within a window).
//------------------------------------------------------------------------------

#ifndef	_VIEW_H
#define	_VIEW_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Font.h>
#include <Handler.h>
#include <InterfaceDefs.h>
#include <Rect.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// view definitions ------------------------------------------------------------

enum {
	B_PRIMARY_MOUSE_BUTTON = 0x01,
	B_SECONDARY_MOUSE_BUTTON = 0x02,
	B_TERTIARY_MOUSE_BUTTON = 0x04
};

enum {
	B_ENTERED_VIEW = 0,
	B_INSIDE_VIEW,
	B_EXITED_VIEW,
	B_OUTSIDE_VIEW
};

enum {
	B_POINTER_EVENTS		= 0x00000001,
	B_KEYBOARD_EVENTS		= 0x00000002
};

enum {
	B_LOCK_WINDOW_FOCUS		= 0x00000001,
	B_SUSPEND_VIEW_FOCUS	= 0x00000002,
	B_NO_POINTER_HISTORY	= 0x00000004
};

enum {
	B_TRACK_WHOLE_RECT,
	B_TRACK_RECT_CORNER
};

enum {
	B_FONT_FAMILY_AND_STYLE	= 0x00000001,
	B_FONT_SIZE				= 0x00000002,
	B_FONT_SHEAR			= 0x00000004,
	B_FONT_ROTATION			= 0x00000008,
	B_FONT_SPACING     		= 0x00000010,
	B_FONT_ENCODING			= 0x00000020,
	B_FONT_FACE				= 0x00000040,
	B_FONT_FLAGS			= 0x00000080,
	B_FONT_ALL				= 0x000000FF
};

const uint32 B_FULL_UPDATE_ON_RESIZE 	= 0x80000000UL;	/* 31 */
const uint32 _B_RESERVED1_ 				= 0x40000000UL;	/* 30 */
const uint32 B_WILL_DRAW 				= 0x20000000UL;	/* 29 */
const uint32 B_PULSE_NEEDED 			= 0x10000000UL;	/* 28 */
const uint32 B_NAVIGABLE_JUMP 			= 0x08000000UL;	/* 27 */
const uint32 B_FRAME_EVENTS				= 0x04000000UL;	/* 26 */
const uint32 B_NAVIGABLE 				= 0x02000000UL;	/* 25 */
const uint32 B_SUBPIXEL_PRECISE 		= 0x01000000UL;	/* 24 */
const uint32 B_DRAW_ON_CHILDREN 		= 0x00800000UL;	/* 23 */
const uint32 B_INPUT_METHOD_AWARE 		= 0x00400000UL;	/* 23 */
const uint32 _B_RESERVED7_ 				= 0x00200000UL;	/* 22 */

/** \brief Flags controlling how to resize/move a view when the parent is resized.
 * \ingroup interface
 * \sa view_flags, BView
 * \author	Kurt Skauen (kurt@atheos.cx), Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

enum view_resize_flags
{
	B_FOLLOW_NONE		= 0x0000, ///< Neither the size nor the position is changed.
	B_FOLLOW_LEFT		= 0x0001, ///< Left edge follows the parents left edge.
	B_FOLLOW_RIGHT		= 0x0002, ///< Right edge follows the parents right edge.
	B_FOLLOW_LEFT_RIGHT	= B_FOLLOW_LEFT | B_FOLLOW_RIGHT,
	B_FOLLOW_TOP		= 0x0004, ///< Top edge follows the parents top edge.
	B_FOLLOW_BOTTOM		= 0x0008, ///< Bottom edge follows the parents bottom edge.
	B_FOLLOW_TOP_BOTTOM = B_FOLLOW_TOP | B_FOLLOW_BOTTOM,
	B_FOLLOW_ALL		= 0x000F, ///< All edges follows the corresponding edge in the parent
	B_FOLLOW_ALL_SIDES	= B_FOLLOW_ALL,
	/**
	* If B_FOLLOW_LEFT is set the right edge follows the parents center.
	* if B_FOLLOW_RIGHT is set the left edge follows the parents center.
	*/
	B_FOLLOW_H_CENTER	= 0x0010,
	/**
	* If B_FOLLOW_TOP is set the bottom edge follows the parents center.
	* if B_FOLLOW_BOTTOM is set the top edge follows the parents center.
	*/
	B_FOLLOW_V_CENTER	= 0x0020,
	B_FOLLOW_SPECIAL	= 0x0040,
	B_FOLLOW_MASK		= 0x007f
};


class BBitmap;
class BCursor;
class BMessage;
class BPicture;
class BPolygon;
class BRegion;
class BScrollBar;
class BScrollView;
class BShape;
class BShelf;
class BString;
class BWindow;
class BFont;
struct _view_attr_;
struct _array_data_;
struct _array_hdr_;
struct overlay_restrictions;

enum
{
	PEN_DETAIL,	      /* primary pen (Draw, PutBixel, ... )					*/
	PEN_BACKGROUND,   /* secondary pen ( EraseRect, text background, ...)	*/
	PEN_SHINE,	      /* bright side of 3D object 							*/
	PEN_SHADOW,	      /* dark side of 3D object								*/

	PEN_BRIGHT,       /* same as PEN_SHINE, but a bit darker				*/
	PEN_DARK,	      /* same as PEN_SHADOW, but a bit brighter				*/
	PEN_WINTITLE,     /* Fill inside window title of a unselected window	*/
	PEN_WINBORDER,    /* Window border fill when unselected					*/

	PEN_SELWINTITLE,  /* Fill inside window title of a selected window		*/
	PEN_SELWINBORDER, /* Window border fill when selected					*/
	PEN_WINDOWTEXT,
	PEN_SELWNDTEXT,

	PEN_WINCLIENT,    /* Used to clear areas inside client area of window	*/
	PEN_GADGETFILL,
	PEN_SELGADGETFILL,
	PEN_GADGETTEXT,

	PEN_SELGADGETTEXT
};

enum default_color_t
{
	COL_NORMAL,
	COL_SHINE,
	COL_SHADOW,
	COL_SEL_WND_BORDER,
	COL_NORMAL_WND_BORDER,
	COL_MENU_TEXT,
	COL_SEL_MENU_TEXT,
	COL_MENU_BACKGROUND,
	COL_SEL_MENU_BACKGROUND,
	COL_SCROLLBAR_BG,
	COL_SCROLLBAR_KNOB,
	COL_LISTVIEW_TAB,
	COL_LISTVIEW_TAB_TEXT,
	COL_COUNT
};

rgb_color get_default_color( default_color_t nColor );
void __set_default_color( default_color_t nColor, const rgb_color& sColor );
void set_default_color( default_color_t nColor, const rgb_color& sColor );

enum
{
	FRAME_RECESSED    = 0x000008,
	FRAME_RAISED      = 0x000010,
	FRAME_THIN	      = 0x000020,
	FRAME_WIDE	      = 0x000040,
	FRAME_ETCHED      = 0x000080,
	FRAME_FLAT	      = 0x000100,
	FRAME_DISABLED    = 0x000200,
	FRAME_TRANSPARENT = 0x010000
};




/** Base class for all GUI components.
 * \ingroup interface
 *
 * The View class is the work horse in the GUI. Before you can render any graphics
 * into a window, you have to create one or more views and attatch them to it.
 * Views can be added to a window, or to another view to create a hierarchy. To render
 * someting into a window you normaly inherit this class and overload the Draw() function.
 * Each view has its own graphical environment consisting of three colors (foreground
 * background and erase), a pen position and the drawing mode. Each view lives in a
 * coordinate system relative to its parent. Views are also responsible for receiving
 * user input. The active view in the active window receives keybord and mouse events.
 *
 *  \sa BWindow, BHandler, \ref view_flags
 *  \author	Kurt Skauen (kurt@atheos.cx), Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

// BView class -----------------------------------------------------------------
class BView : public BHandler {

public:
							BView(BRect frame, const char* name,
								  uint32 resizeMask, uint32 flags);
	virtual					~BView();

							BView(BMessage* data);
	static	BArchivable*	Instantiate(BMessage* data);
	virtual	status_t		Archive(BMessage* data, bool deep = true) const;

	virtual	void			AttachedToWindow();
	virtual	void			AllAttached();
	virtual	void			DetachedFromWindow();
	virtual	void			AllDetached();

	BView*			ChildAt( const BPoint& cPos ) const;

			void			AddChild(BView* child, BView* before = NULL);
			bool			RemoveChild(BView* child);
			int32			CountChildren() const;
			BView*			ChildAt(int32 index) const;
			BView*			NextSibling() const;
			BView*			PreviousSibling() const;
			bool			RemoveSelf();

			BWindow			*Window() const;

	virtual	void			Draw(BRect updateRect);
	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseUp(BPoint where);
	virtual	void			MouseMoved(BPoint where,
									   uint32 code,
									   const BMessage* a_message);
	virtual	void			WindowActivated(bool state);
	virtual	void			KeyDown(const char* bytes, int32 numBytes);
	virtual	void			KeyUp(const char* bytes, int32 numBytes);
	virtual	void			Pulse();
	virtual	void			FrameMoved(BPoint new_position);
	virtual	void			FrameResized(float new_width, float new_height);

	virtual	void			TargetedByScrollView(BScrollView* scroll_view);
			void			BeginRectTracking(BRect startRect,
											  uint32 style = B_TRACK_WHOLE_RECT);
			void			EndRectTracking();

			void			GetMouse(BPoint* location,
									 uint32* buttons,
									 bool checkMessageQueue = true);

	void			DragMessage(BMessage* aMessage, BPoint cOffset,
										BRect dragRect,
								BHandler* reply_to = NULL );

			void			DragMessage(BMessage* aMessage,
										BBitmap* anImage,
										BPoint offset,
										BHandler* reply_to = NULL);

			BView*			FindView(const char* name) const;
			BView*			Parent() const;
			BRect			Bounds() const;
			BRect			Frame() const;
			void			ConvertToScreen(BPoint* pt) const;
			BPoint			ConvertToScreen(BPoint pt) const;
			void			ConvertFromScreen(BPoint* pt) const;
			BPoint			ConvertFromScreen(BPoint pt) const;
			void			ConvertToScreen(BRect* r) const;
			BRect			ConvertToScreen(BRect r) const;
			void			ConvertFromScreen(BRect* r) const;
			BRect			ConvertFromScreen(BRect r) const;
			void			ConvertToParent(BPoint* pt) const;
			BPoint			ConvertToParent(BPoint pt) const;
			void			ConvertFromParent(BPoint* pt) const;
			BPoint			ConvertFromParent(BPoint pt) const;
			void			ConvertToParent(BRect* r) const;
			BRect			ConvertToParent(BRect r) const;
			void			ConvertFromParent(BRect* r) const;
			BRect			ConvertFromParent(BRect r) const;
			BPoint			LeftTop() const;

	virtual	void			SetDrawingMode(drawing_mode mode);
			drawing_mode 	DrawingMode() const;

	virtual	void			SetPenSize(float size);
			float			PenSize() const;

			void			SetViewCursor(const BCursor* cursor, bool sync=true);

	virtual	void			SetViewColor(rgb_color c);
			void			SetViewColor(uchar r, uchar g, uchar b, uchar a = 255);
			rgb_color		ViewColor() const;

	virtual	void			SetHighColor(rgb_color a_color);
			void			SetHighColor(uchar r, uchar g, uchar b, uchar a = 255);
			rgb_color		HighColor() const;

	virtual	void			SetLowColor(rgb_color a_color);
			void			SetLowColor(uchar r, uchar g, uchar b, uchar a = 255);
			rgb_color		LowColor() const;

			void			SetOrigin(BPoint pt);
			void			SetOrigin(float x, float y);
			BPoint			Origin() const;

			void			PushState();
			void			PopState();

			void			MovePenTo(BPoint pt);
			void			MovePenTo(float x, float y);
			void			MovePenBy(float x, float y);
			BPoint			PenLocation() const;
			void			StrokeLine(BPoint toPt,
									   pattern p = B_SOLID_HIGH);
			void			StrokeLine(BPoint pt0,
									   BPoint pt1,
									   pattern p = B_SOLID_HIGH);
			void			BeginLineArray(int32 count);
			void			AddLine(BPoint pt0, BPoint pt1, rgb_color col);
			void			EndLineArray();

			void			StrokePolygon(const BPolygon* aPolygon,
										  bool closed = true,
										  pattern p = B_SOLID_HIGH);
			void			StrokePolygon(const BPoint* ptArray,
										  int32 numPts,
										  bool closed = true,
										  pattern p = B_SOLID_HIGH);
			void			StrokePolygon(const BPoint* ptArray,
										  int32 numPts,
										  BRect bounds,
										  bool closed = true,
										  pattern p = B_SOLID_HIGH);
			void			FillPolygon(const BPolygon* aPolygon,
										pattern p = B_SOLID_HIGH);
			void			FillPolygon(const BPoint* ptArray,
										int32 numPts,
										pattern p = B_SOLID_HIGH);
			void			FillPolygon(const BPoint* ptArray,
										int32 numPts,
										BRect bounds,
										pattern p = B_SOLID_HIGH);

			void			StrokeTriangle(BPoint pt1,
										   BPoint pt2,
										   BPoint pt3,
										   BRect bounds,
										   pattern p = B_SOLID_HIGH);
			void			StrokeTriangle(BPoint pt1,
										   BPoint pt2,
										   BPoint pt3,
										   pattern p = B_SOLID_HIGH);
			void			FillTriangle(BPoint pt1,
										 BPoint pt2,
										 BPoint pt3,
										 pattern p = B_SOLID_HIGH);
			void			FillTriangle(BPoint pt1,
										 BPoint pt2,
										 BPoint pt3,
										 BRect bounds,
										 pattern p = B_SOLID_HIGH);

			void			StrokeRect(BRect r, pattern p = B_SOLID_HIGH);
			void			FillRect(BRect r, pattern p = B_SOLID_HIGH);
			void			FillRegion(BRegion* a_region, pattern p= B_SOLID_HIGH);
			void			InvertRect(BRect r);

			void			StrokeRoundRect(BRect r,
											float xRadius,
											float yRadius,
											pattern p = B_SOLID_HIGH);
			void			FillRoundRect(BRect r,
										  float xRadius,
										  float yRadius,
										  pattern p = B_SOLID_HIGH);

			void			StrokeEllipse(BPoint center,
										  float xRadius,
										  float yRadius,
										  pattern p = B_SOLID_HIGH);
			void			StrokeEllipse(BRect r, pattern p = B_SOLID_HIGH);
			void			FillEllipse(BPoint center,
										float xRadius,
										float yRadius,
										pattern p = B_SOLID_HIGH);
			void			FillEllipse(BRect r, pattern p = B_SOLID_HIGH);

			void			StrokeArc(BPoint center,
									  float xRadius,
									  float yRadius,
									  float start_angle,
									  float arc_angle,
									  pattern p = B_SOLID_HIGH);
			void			StrokeArc(BRect r,
									  float start_angle,
									  float arc_angle,
									  pattern p = B_SOLID_HIGH);
			void			FillArc(BPoint center,
									float xRadius,
									float yRadius,
									float start_angle,
									float arc_angle,
									pattern p = B_SOLID_HIGH);
			void			FillArc(BRect r,
									float start_angle,
									float arc_angle,
									pattern p = B_SOLID_HIGH);

			void			StrokeBezier(BPoint* controlPoints,
										 pattern p = B_SOLID_HIGH);
			void			FillBezier(BPoint* controlPoints,
									   pattern p = B_SOLID_HIGH);
	
			void			StrokeShape(BShape* shape,
										pattern p = B_SOLID_HIGH);
			void			FillShape(BShape* shape,
									  pattern p = B_SOLID_HIGH);

			void			CopyBits(BRect src, BRect dst);
			void			DrawBitmapAsync(const BBitmap* aBitmap,
											BRect srcRect,
											BRect dstRect);
			void			DrawBitmapAsync(const BBitmap* aBitmap);
			void			DrawBitmapAsync(const BBitmap* aBitmap, BPoint where);
			void			DrawBitmapAsync(const BBitmap* aBitmap, BRect dstRect);
			void			DrawBitmap(const BBitmap* aBitmap,
									   BRect srcRect,
									   BRect dstRect);
			void			DrawBitmap(const BBitmap* aBitmap);
			void			DrawBitmap(const BBitmap* aBitmap, BPoint where);
			void			DrawBitmap(const BBitmap* aBitmap, BRect dstRect);

			void			DrawChar(char aChar);
			void			DrawChar(char aChar, BPoint location);
			void			DrawString(const char* aString,
									   escapement_delta* delta = NULL);
			void			DrawString(const char* aString, BPoint location,
									   escapement_delta* delta = NULL);
			void			DrawString(const char* aString, int32 length,
									   escapement_delta* delta = NULL);
			void			DrawString(const char* aString,
									   int32 length,
									   BPoint location,
									   escapement_delta* delta = 0L);

	virtual void            SetFont(BFont* font, uint32 mask = B_FONT_ALL);
	BFont*			GetFont(void) const;
			void            GetFont(BFont* font) const;

			void			TruncateString(BString* in_out,
										   uint32 mode,
										   float width) const;
			float			StringWidth(const char* string) const;
			float			StringWidth(const char* string, int32 length) const;
			void			SetFontSize(float size);
			void			GetFontHeight(font_height* height) const;

	void			Invalidate(BRect cRect, bool bRecurse = false);
	void			Invalidate(bool bRecurse = false);

			status_t		SetEventMask(uint32 mask, uint32 options=0);
			uint32			EventMask();
			status_t		SetMouseEventMask(uint32 mask, uint32 options=0);

	virtual	void			SetFlags(uint32 flags);
			uint32			Flags(uint32 nMask = ~0L) const;
	virtual	void			SetResizingMode(uint32 mode);
			uint32			ResizingMode() const;

			void			MoveBy(float dh, float dv);
			void			MoveTo(BPoint where);
			void			MoveTo(float x, float y);
			void			ResizeBy(float dh, float dv);
			void			ResizeTo(float width, float height);
			void			ScrollBy(float dh, float dv);
			void			ScrollTo(float x, float y);
	virtual	void			ScrollTo(BPoint where);
	virtual	void			MakeFocus(bool focusState = true);
			bool			IsFocus() const;

	virtual	void			Show();
	virtual	void			Hide();
			bool			IsHidden() const;
			bool			IsHidden(const BView* looking_from) const;

			void			Flush() const;
			void			Sync() const;

	virtual	void			GetPreferredSize(float* width, float* height);
	virtual	void			ResizeToPreferred();

			BScrollBar*		ScrollBar(orientation posture) const;

	virtual BHandler*		ResolveSpecifier(BMessage* msg,
											 int32 index,
											 BMessage* specifier,
											 int32 form,
											 const char* property);
	virtual status_t		GetSupportedSuites(BMessage* data);

			bool			IsPrinting() const;
			void			SetScale(float scale) const;

// Begin Cosmoe-specific functions

	void			FillRect( BRect cRect, rgb_color sColor );	// WARNING: Will leave HiColor at sColor

	void			MoveBy( BPoint cDelta );
	void			ResizeBy( const BPoint& cDelta );
	void			ResizeTo( const BPoint& cSize );

	virtual void	ViewScrolled( const BPoint& cDelta );
	virtual void	FontChanged( BFont* pcNewFont );

	virtual BPoint	GetContentSize() const;

	const char*		Title() const;

	virtual int		GetTabOrder();
	virtual void	SetTabOrder( int nOrder );

	void			SetMousePos( const BPoint& cPosition );
	
	void			WheelMoved( BPoint& inPoint );


	BRect			GetNormalizedBounds() const;
	float			Width() const;
//	float			Height() const;

	virtual void	SetFrame( const BRect& cRect, bool bNotifyServer = true );

	virtual int		ToggleDepth();

	BPoint			ConvertToWindow( BPoint cPoint ) const;
	void			ConvertToWindow( BPoint* cPoint ) const;
	BRect			ConvertToWindow( BRect cRect ) const;
	void			ConvertToWindow( BRect* cRect ) const;

	BPoint			ConvertFromWindow( BPoint cPoint ) const;
	void			ConvertFromWindow( BPoint* cPoint ) const;
	BRect			ConvertFromWindow( BRect cRect ) const;
	void			ConvertFromWindow( BRect* cRect ) const;

	BPoint			GetScrollOffset() const;

	void			EraseRect( const BRect& cRect );
	void			DrawFrame( const BRect& cRect, uint32 nFlags );

	// Font functions.
	void            GetTruncatedStrings( const char** pazStringArray,
										 int    nStringCount,
										 uint32 nMode,
										 float  nWidth,
										 char** pazResultArray ) const;

	void			GetStringWidths( const char** apzStringArray, const int32* anLengthArray,
					int32 nStringCount, float* avWidthArray ) const;

	int				GetStringLength( const char* pzString, float vWidth, bool bIncludeLast = false ) const;
	int				GetStringLength( const char* pzString, int nLength, float vWidth, bool bIncludeLast = false ) const;
	int				GetStringLength( const std::string& cString, float vWidth, bool bIncludeLast = false ) const;
	void			GetStringLengths( const char** apzStringArray, const int* anLengthArray, int nStringCount,
									  float vWidth, int* anMaxLengthArray, bool bIncludeLast = false ) const;



	void			Ping( int nSize = 0 );

// Private or reserved ---------------------------------------------------------
	virtual status_t		Perform(perform_code d, void* arg);

	virtual	void			DrawAfterChildren(BRect r);

private:

	friend class BScrollBar;
	friend class BWindow;
	friend class BBitmap;
	friend class BFont;
	friend class BShelf;
	friend class BTabView;
	friend class TopView;

	static int		s_nNextTabOrder;


	void			show_view( bool bVisible = true );

	void			_LinkChild( BView* pcChild, bool bTopmost ); // Add a view to the child list
	void			_UnlinkChild( BView* pcChild );				 // Remove a view from the child list
	void			_Attached( BWindow* pcFrame, BView* pcParent, int hHandle, int nHideCount );
	void			_AttachedTree( BWindow* pcFrame, BView* pcParent, int hHandle, int nHideCount );
	void			_NotifyAttachedToWindow();
	void			_NotifyAllAttached();
	void			_Detached( bool bFirst, int nHideCount );
	void 			_IncHideCount( bool bVisible );
	int				_GetHandle() const;

	void			_SetMouseMoveRun( int nValue );
	int				_GetMouseMoveRun() const;

	void			_SetMouseMode( int nMode );
	int				_GetMouseMode() const;

	void			_SetHScrollBar( BScrollBar* pcScrollBar );
	void			_SetVScrollBar( BScrollBar* pcScrollBar );

	void			_BeginUpdate();
	void			_EndUpdate();

	void			_ParentSized( const BPoint& cDelta );
	void			_WindowActivated( bool bIsActive );
	void			_ReleaseFont();
	void			_ConstrictRectangle( BRect* pcRect, const BPoint& cOffset );

	virtual	void			_ReservedView2();
	virtual	void			_ReservedView3();
	virtual	void			_ReservedView4();
	virtual	void			_ReservedView5();
	virtual	void			_ReservedView6();
	virtual	void			_ReservedView7();
	virtual	void			_ReservedView8();

#if !_PR3_COMPATIBLE_
	virtual	void			_ReservedView9();
	virtual	void			_ReservedView10();
	virtual	void			_ReservedView11();
	virtual	void			_ReservedView12();
	virtual	void			_ReservedView13();
	virtual	void			_ReservedView14();
	virtual	void			_ReservedView15();
	virtual	void			_ReservedView16();
#endif

	class Private;
    Private* m;

			int32			server_token;
			uint32			f_type;
			float			origin_h;
			float			origin_v;
			BWindow*		owner;			// used
			BView*			parent;			// used
			BView*			next_sibling;	// used
			BView*			prev_sibling;	// used
			BView*			first_child;	// used

			int16 			fShowLevel;		// used
			bool			top_level_view;	// used
			bool			fNoISInteraction;

			BScrollBar*		fVerScroller;	// used
			BScrollBar*		fHorScroller;	// used
			bool			f_is_printing;
			bool			attached;
			bool			_unused_bool1;
			bool			_unused_bool2;
			uint32			fEventMask;
			uint32			fEventOptions;
			uint32			_reserved[4];
#if !_PR3_COMPATIBLE_
			uint32			_more_reserved[3];
#endif
};
//------------------------------------------------------------------------------


// inline definitions ----------------------------------------------------------
inline void	BView::ScrollTo(float x, float y)
{
	ScrollTo(BPoint(x, y));
}
//------------------------------------------------------------------------------
inline void	BView::SetViewColor(uchar r, uchar g, uchar b, uchar a)
{
	rgb_color	a_color;
	a_color.red = r;		a_color.green = g;
	a_color.blue = b;		a_color.alpha = a;
	SetViewColor(a_color);
}
//------------------------------------------------------------------------------
inline void	BView::SetHighColor(uchar r, uchar g, uchar b, uchar a)
{
	rgb_color	a_color;
	a_color.red = r;		a_color.green = g;
	a_color.blue = b;		a_color.alpha = a;
	SetHighColor(a_color);
}
//------------------------------------------------------------------------------
inline void	BView::SetLowColor(uchar r, uchar g, uchar b, uchar a)
{
	rgb_color	a_color;
	a_color.red = r;		a_color.green = g;
	a_color.blue = b;		a_color.alpha = a;
	SetLowColor(a_color);
}
//------------------------------------------------------------------------------

#endif	// _VIEW_H

/*
 @log
	* added PrintToStream() method for debugging BView.
 
 */

