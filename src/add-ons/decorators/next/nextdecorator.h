/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_NEXTDECORATOR_H__
#define __F_NEXTDECORATOR_H__

#include <Decorator.h>
#include <Font.h>
#include <Rect.h>

#include <string>



class NextDecorator : public Decorator
{
public:
			NextDecorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 nWndFlags );
	virtual ~NextDecorator();

	virtual click_type Clicked(BPoint pt, int32 buttons, int32 modifiers);
	virtual void FrameSized( const BRect& cNewFrame );
	virtual BRect GetBorderSize();
	virtual BPoint GetMinimumSize();
	virtual void SetFlags( uint32 nFlags );
	virtual void FontChanged();
	virtual void SetTitle( const char* pzTitle );
	virtual void SetCloseButtonState( bool bPushed );
	virtual void SetZoomButtonState( bool bPushed );
	virtual void Draw(BRect cUpdateRect);
private:
	void CalculateBorderSizes();

	virtual void _DrawClose(BRect r);
	virtual void _DrawZoom(BRect r);
	virtual void _SetFocus(void);
	virtual void _DoLayout();

	font_height m_sFontHeight;

	float  m_vLeftBorder;
	float  m_vTopBorder;
	float  m_vRightBorder;
	float  m_vBottomBorder;
};

#endif // __F_NEXTDECORATOR_H__
