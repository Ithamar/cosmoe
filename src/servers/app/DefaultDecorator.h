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
//	File Name:		DefaultDecorator.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Fallback decorator for the app_server
//  
//------------------------------------------------------------------------------
#ifndef _DEFAULT_DECORATOR_H_
#define _DEFAULT_DECORATOR_H_

#include "Decorator.h"
#include <Region.h>
#include <Font.h>
#include <Rect.h>

class DefaultDecorator: public Decorator
{
public:
							DefaultDecorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 nWndFlags);
	virtual					~DefaultDecorator(void);

	virtual void FrameSized( const BRect& cNewFrame );
	virtual BRect GetBorderSize();
	virtual BPoint GetMinimumSize();
	virtual void SetFlags( uint32 nFlags );
	virtual void FontChanged();
	virtual void SetTitle( const char* pzTitle );
	virtual void SetCloseButtonState( bool bPushed );
	virtual void SetZoomButtonState( bool bPushed );
	virtual void Draw(BRect r);
	virtual	click_type		Clicked(BPoint pt, int32 buttons, int32 modifiers);

protected:
	virtual void _DrawClose(BRect r);
	virtual void _DrawZoom(BRect r);
	virtual void _SetFocus(void);
	virtual void _DoLayout(void);
	void CalculateBorderSizes();

private:
	font_height m_sFontHeight;

	float  m_vLeftBorder;
	float  m_vTopBorder;
	float  m_vRightBorder;
	float  m_vBottomBorder;
};

#endif
