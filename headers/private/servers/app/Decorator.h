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
//	File Name:		Decorator.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Base class for window decorators
//  
//------------------------------------------------------------------------------
#ifndef _DECORATOR_H_
#define _DECORATOR_H_

#include <SupportDefs.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>
#include "ColorSet.h"

#include <string>

class Layer;


const float DECORATOR_APIVERSION = 6.0f;

typedef enum { DEC_NONE=0, DEC_ZOOM, DEC_CLOSE, DEC_MINIMIZE,
	DEC_TAB, DEC_DRAG, DEC_MOVETOBACK, DEC_MOVETOFRONT, DEC_SLIDETAB,
	
	DEC_RESIZE, CLICK_RESIZE_L, CLICK_RESIZE_T, 
	CLICK_RESIZE_R, CLICK_RESIZE_B, CLICK_RESIZE_LT, CLICK_RESIZE_RT, 
	CLICK_RESIZE_LB, CLICK_RESIZE_RB } click_type;

class Decorator
{
public:
	Decorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 wflags);
	virtual ~Decorator(void);

	void SetColors(const ColorSet &cset);
	void SetFeel(int32 wfeel);
	void SetLook(int32 wlook);

	void SetClose(bool is_down);
	void SetMinimize(bool is_down);
	void SetZoom(bool is_down);
	virtual void SetTitle(const char *string);

	int32 GetLook(void);
	int32 GetFeel(void);
	uint32 GetFlags(void);
	const char *GetTitle(void);
		// we need to know its border(frame). WinBorder's _frame rect
		// must expand to include Decorator borders. Otherwise we can't
		// draw the border. We also add GetTabRect because I feel we'll need it
	BRect GetBorderRect(void);
	BRect GetTabRect(void);

	bool GetClose(void);
	bool GetMinimize(void);
	bool GetZoom(void);


	void SetFocus(bool is_active);
	bool GetFocus(void) { return _has_focus; };
	ColorSet GetColors(void) { return (_colors)?*_colors:ColorSet(); }

	Layer* GetLayer() const;

	virtual void GetFootprint(BRegion *region);
	virtual click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);

	virtual void FrameSized( const BRect& cNewFrame ) = 0;
	virtual BRect GetBorderSize() = 0;
	virtual BPoint GetMinimumSize() = 0;
	virtual void SetFlags( uint32 nFlags ) = 0;
	virtual void FontChanged() = 0;
	virtual void SetCloseButtonState( bool bPushed ) = 0;
	virtual void SetZoomButtonState( bool bPushed ) = 0;

	virtual void Draw(BRect r);

protected:
	/*!
		\brief Returns the number of characters in the title
		\return The title character count
	*/
	int32 _TitleWidth(void) { return (_title_string)?_title_string->CountChars():0; }

	virtual void _DrawClose(BRect r) = 0;
	virtual void _DrawZoom(BRect r) = 0;
	virtual void _SetFocus(void);
	virtual void _DoLayout(void);
	virtual void _SetColors(void);

	ColorSet *_colors;
	uint32 _flags;
	int32 _look, _feel;
	BRect _zoomrect,_closerect,_minimizerect,_tabrect,_frame,
		_resizerect,_borderrect;

private:
	Layer* m_pcLayer;
	bool _close_state, _zoom_state, _minimize_state;
	bool _has_focus;
	BString *_title_string;
};

typedef float get_version(void);
typedef Decorator *create_decorator(Layer* pcView, int32 wlook, int32 wfeel, uint32 wflags);


#endif
