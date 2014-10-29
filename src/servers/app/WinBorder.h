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
//	File Name:		WinBorder.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Layer subclass which handles window management
//  
//------------------------------------------------------------------------------
#ifndef _WINBORDER_H_
#define _WINBORDER_H_

#include <Rect.h>
#include <String.h>
#include "Layer.h"

class ServerWindow;
class Decorator;
class DisplayDriver;
class Desktop;

class PointerEvent
{
	public:
	int32 code;	//B_MOUSE_UP, B_MOUSE_DOWN, B_MOUSE_MOVED
			//B_MOUSE_WHEEL_CHANGED
	bigtime_t when;
	BPoint where;
	float wheel_delta_x;
	float wheel_delta_y;
	int32 modifiers;
	int32 buttons;	//B_PRIMARY_MOUSE_BUTTON, B_SECONDARY_MOUSE_BUTTON
			//B_TERTIARY_MOUSE_BUTTON
	int32 clicks;
};

class WinBorder : public Layer
{
public:
	WinBorder( ServerWindow* pcWindow, Layer* pcParent, const char* pzName, bool bBackdrop );

	void                 SetDecorator( Decorator* pcDecorator );
	void                 SetFlags( uint32 nFlags );
	Layer*               GetClient() const { return( fTopLayer ); }
	void                 SetSizeLimits( const BPoint& cMinSize, const BPoint& cMaxSize );
	void                 GetSizeLimits( BPoint* pcMinSize, BPoint* pcMaxSize );
	void                 SetAlignment( const IPoint& cSize, const IPoint& cSizeOffset, const IPoint& cPos, const IPoint& cPosOffset );

	virtual void         SetFrame( const BRect& cRect );
	virtual void         RequestDraw(const IRect& cUpdateRect, bool bUpdate = false );

	bool                 MouseDown( BMessenger* pcAppTarget, const BPoint& cPos, int nButton );
	bool                 MouseMoved( BMessenger* pcAppTarget, const BPoint& cNewPos, int nTransit );
	void                 MouseUp( BMessenger* pcAppTarget, const BPoint& cPos, int nButton );

	void                 WndMoveReply( BMessenger* pcAppTarget );
	bool                 HasPendingSizeEvents() const;
private:
	IPoint        GetHitPointBase() const;
	IRect         AlignRect( const IRect& cRect, const IRect& cBorders );
	void          DoSetFrame( const BRect& cRect );

	Decorator *fDecorator;
	Layer *fTopLayer;
	IPoint                    m_cHitOffset;
	click_type m_eHitItem; // Item hit by the intital mouse-down event.
	click_type m_eCursorHitItem;
	int                       m_nCloseDown;
	int                       m_nZoomDown;
	bool                      m_bWndMovePending;
	bool                      m_bFrameUpdated;
	IPoint                    m_cMinSize;
	IPoint                    m_cMaxSize;

	IRect                     m_cRawFrame;
	IPoint                    m_cAlignSize;
	IPoint                    m_cAlignSizeOff;
	IPoint                    m_cAlignPos;
	IPoint                    m_cAlignPosOff;

	IPoint                    m_cDeltaMove;
	IPoint                    m_cDeltaSize;
};

#endif
