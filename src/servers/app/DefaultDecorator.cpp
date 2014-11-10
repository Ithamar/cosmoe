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
//	File Name:		DefaultDecorator.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Fallback decorator for the app_server
//  
//------------------------------------------------------------------------------
#include <Rect.h>
#include <View.h>
#include "LayerData.h"
#include "DefaultDecorator.h"
#include "PatternHandler.h"
#include <stdio.h>

#include "Layer.h"
#include <Window.h>
#include <macros.h>

//#define USE_VIEW_FILL_HACK

//#define DEBUG_DECORATOR

#ifdef DEBUG_DECORATOR
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

DefaultDecorator::DefaultDecorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 wflags)
 : Decorator(pcLayer,wlook,wfeel,wflags)
{
	m_sFontHeight = pcLayer->GetFontHeight();

	CalculateBorderSizes();

	STRACE(("DefaultDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",rect.left,rect.top,rect.right,rect.bottom));
}


DefaultDecorator::~DefaultDecorator(void)
{
	STRACE(("DefaultDecorator: ~DefaultDecorator()\n"));
}

click_type DefaultDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	#ifdef DEBUG_DECORATOR
	printf("DefaultDecorator: Clicked\n");
	printf("\tPoint: (%.1f,%.1f)\n",pt.x,pt.y);
	printf("\tButtons:\n");
	if(buttons==0)
		printf("\t\tNone\n");
	else
	{
		if(buttons & B_PRIMARY_MOUSE_BUTTON)
			printf("\t\tPrimary\n");
		if(buttons & B_SECONDARY_MOUSE_BUTTON)
			printf("\t\tSecondary\n");
		if(buttons & B_TERTIARY_MOUSE_BUTTON)
			printf("\t\tTertiary\n");
	}
	printf("\tModifiers:\n");
	if(modifiers==0)
		printf("\t\tNone\n");
	else
	{
		if(modifiers & B_CAPS_LOCK)
			printf("\t\tCaps Lock\n");
		if(modifiers & B_NUM_LOCK)
			printf("\t\tNum Lock\n");
		if(modifiers & B_SCROLL_LOCK)
			printf("\t\tScroll Lock\n");
		if(modifiers & B_LEFT_COMMAND_KEY)
			printf("\t\t Left Command\n");
		if(modifiers & B_RIGHT_COMMAND_KEY)
			printf("\t\t Right Command\n");
		if(modifiers & B_LEFT_CONTROL_KEY)
			printf("\t\tLeft Control\n");
		if(modifiers & B_RIGHT_CONTROL_KEY)
			printf("\t\tRight Control\n");
		if(modifiers & B_LEFT_OPTION_KEY)
			printf("\t\tLeft Option\n");
		if(modifiers & B_RIGHT_OPTION_KEY)
			printf("\t\tRight Option\n");
		if(modifiers & B_LEFT_SHIFT_KEY)
			printf("\t\tLeft Shift\n");
		if(modifiers & B_RIGHT_SHIFT_KEY)
			printf("\t\tRight Shift\n");
		if(modifiers & B_MENU_KEY)
			printf("\t\tMenu\n");
	}
	#endif

	// In checking for hit test stuff, we start with the smallest rectangles the user might
	// be clicking on and gradually work our way out into larger rectangles.
	if(_closerect.Contains(pt))
		return DEC_CLOSE;

	if(_zoomrect.Contains(pt))
		return DEC_ZOOM;

	// Clicking in the tab?
	if(_tabrect.Contains(pt))
	{
		// Here's part of our window management stuff
		if(buttons==B_SECONDARY_MOUSE_BUTTON)
			return DEC_MOVETOBACK;
		return DEC_DRAG;
	}

	if ( pt.x < 4 )
	{
		if ( pt.y < 4 )
		{
			return( CLICK_RESIZE_LT );
		}
		else if ( pt.y > _frame.bottom - 4 )
		{
			return( CLICK_RESIZE_LB );
		}
		else
		{
			return( CLICK_RESIZE_L );
		}
	}
	else if ( pt.x > _frame.right - 4 )
	{
		if ( pt.y < 4 )
		{
			return( CLICK_RESIZE_RT );
		}
		else if ( pt.y > _frame.bottom - 4 )
		{
			return( CLICK_RESIZE_RB );
		}
		else
		{
			return( CLICK_RESIZE_R );
		}
	}
	else if ( pt.y < 4 )
	{
		return( CLICK_RESIZE_T );
	}
	else if ( pt.y > _frame.bottom - 4 )
	{
		return( CLICK_RESIZE_B );
	}
	
	// Guess user didn't click anything
	return DEC_NONE;
}


void DefaultDecorator::CalculateBorderSizes()
{
	if ( _look & B_NO_BORDER_WINDOW_LOOK )
	{
		m_vLeftBorder   = 0;
		m_vRightBorder  = 0;
		m_vTopBorder    = 0;
		m_vBottomBorder = 0;
	}
	else
	{
		if ( (_look & B_TITLED_WINDOW_LOOK)   ||
			 (_look & B_DOCUMENT_WINDOW_LOOK) ||
			 (_look & B_FLOATING_WINDOW_LOOK) )
		{
			m_vTopBorder = float(m_sFontHeight.ascent + m_sFontHeight.descent + 6);
		}
		else
		{
			m_vTopBorder = 4;
		}
		m_vLeftBorder   = 4;
		m_vRightBorder  = 4;
		m_vBottomBorder = 4;
	}
}


void DefaultDecorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	Decorator::SetFlags(nFlags);
	CalculateBorderSizes();
	pcView->Invalidate();
	_DoLayout();
}


void DefaultDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	_DoLayout();
}


BPoint DefaultDecorator::GetMinimumSize()
{
	BPoint cMinSize( 0.0f, m_vTopBorder + m_vBottomBorder );

	if ( (_flags & B_NOT_CLOSABLE) == 0 )
	{
		cMinSize.x += _closerect.Width();
	}
	
	if ( (_flags & B_NOT_ZOOMABLE) == 0 )
	{
		cMinSize.x += _zoomrect.Width();
	}
	
	if ( cMinSize.x < m_vLeftBorder + m_vRightBorder )
	{
		cMinSize.x = m_vLeftBorder + m_vRightBorder;
	}
	
	return( cMinSize );
}



BRect DefaultDecorator::GetBorderSize()
{
	return( BRect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}


void DefaultDecorator::SetTitle( const char* pzTitle )
{
	Decorator::SetTitle(pzTitle);
	Draw( _frame );
}

void DefaultDecorator::_SetFocus(void)
{
	Draw( _frame );
}


void DefaultDecorator::FrameSized( const BRect& cFrame )
{
	Layer* pcView = GetLayer();
	BPoint cDelta( cFrame.Width() - _frame.Width(), cFrame.Height() - _frame.Height() );

	_frame = cFrame;
	_frame.OffsetTo(0, 0);

	_DoLayout();

	if ( cDelta.x != 0.0f )
	{
		BRect cDamage = _frame;

		cDamage.left = _zoomrect.left - fabs(cDelta.x)  - 2.0f;
		pcView->Invalidate( cDamage );
	}

	if ( cDelta.y != 0.0f )
	{
		BRect cDamage = _frame;

		cDamage.top = cDamage.bottom - max_c( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
		pcView->Invalidate( cDamage );
	}
}


void DefaultDecorator::_DoLayout(void)
{
STRACE(("DefaultDecorator: Do Layout\n"));
	// Here we determine the size of every rectangle that we use
	// internally when we are given the size of the client rectangle.
	
	// Current version simply makes everything fit inside the rect
	// instead of building around it. This will change.

	if ( _flags & B_NOT_CLOSABLE )
	{
		_closerect.left   = 0;
		_closerect.right  = 0;
		_closerect.top    = 0;
		_closerect.bottom = 0;
	}
	else
	{
		_closerect.left = 0;
		_closerect.right = m_vTopBorder - 1;
		_closerect.top = 0;
		_closerect.bottom = m_vTopBorder - 1;
	}

	if ( _flags & B_NOT_ZOOMABLE )
	{
		_zoomrect.left  = 0;
		_zoomrect.right = 0;
	}
	else
	{
		_zoomrect.right = _frame.right;
		_zoomrect.left  = ceil( _zoomrect.right - m_vTopBorder * 1.5f);
	}

	_zoomrect.top    = 0;
	_zoomrect.bottom = m_vTopBorder - 1;

	if ( _flags & B_NOT_CLOSABLE )
	{
		_tabrect.left  = 0.0f;
	}
	else
	{
		_tabrect.left  = _closerect.right + 1.0f;
	}

	if ( _flags & B_NOT_ZOOMABLE )
	{
		_tabrect.right = _frame.right;
	}
	else
	{
		_tabrect.right = _zoomrect.left - 1.0f;
	}

	_tabrect.top    = 0;
	_tabrect.bottom = m_vTopBorder - 1;
}


void DefaultDecorator::SetCloseButtonState(bool bPushed)
{
	SetClose(bPushed);
	if ( (_flags & B_NOT_CLOSABLE) == 0 )
	{
		_DrawClose(_closerect);
	}
}


void DefaultDecorator::SetZoomButtonState( bool bPushed )
{
	SetZoom(bPushed);
	if ( (_flags & B_NOT_ZOOMABLE) == 0 )
	{
		_DrawZoom(_zoomrect);
	}
}



void DefaultDecorator::_DrawZoom(BRect r)
{
STRACE(("_DrawZoom(%f,%f,%f,%f)\n", r.left, r.top, r.right, r.bottom));
	// If this has been implemented, then the decorator has a Zoom button
	// which should be drawn based on the state of the member zoomstate
	rgb_color sFillColor =  GetFocus() ? GetDefaultColor( PEN_SELWINTITLE ) : GetDefaultColor( PEN_WINTITLE );
	BRect        L,R;

	L.left = r.left + ((r.Width()+1.0f) / 6);
	L.top  = r.top  + ((r.Height()+1.0f) / 6);

	L.right  = r.right  - ((r.Width()+1.0f) / 6);
	L.bottom = r.bottom - ((r.Height()+1.0f) / 6);

	R.left = L.left + 1;
	R.top  = L.top  + 1;

	R.right = R.left + ((r.Width()+1.0f) / 3);
	R.bottom = R.top + ((r.Height()+1.0f) / 3);

	Layer* pcView = GetLayer();

	if ( GetZoom() )
	{
		pcView->FillRect( r, sFillColor );
		pcView->DrawFrame( r, FRAME_RECESSED | FRAME_TRANSPARENT );
		pcView->DrawFrame( R, FRAME_RAISED | FRAME_TRANSPARENT  );
		pcView->DrawFrame( L, FRAME_RECESSED | FRAME_TRANSPARENT );

		pcView->FillRect( BRect( L.left + 1, L.top + 1, L.right - 1, L.bottom - 1 ), sFillColor );
	}
	else
	{
		pcView->FillRect( r, sFillColor );
		pcView->DrawFrame( r, FRAME_RAISED | FRAME_TRANSPARENT );
		pcView->DrawFrame( L, FRAME_RAISED | FRAME_TRANSPARENT );
		pcView->DrawFrame( R, FRAME_RECESSED | FRAME_TRANSPARENT  );
	}
}


void DefaultDecorator::_DrawClose(BRect r)
{
	rgb_color sFillColor =  GetFocus() ? GetDefaultColor( PEN_SELWINTITLE ) : GetDefaultColor( PEN_WINTITLE );
	BRect L;

	L.left = r.left + ((r.Width()+1.0f) / 3);
	L.top  = r.top  + ((r.Height()+1.0f) / 3);

	L.right  = r.right  - ((r.Width()+1.0f) / 3);
	L.bottom = r.bottom - ((r.Height()+1.0f) / 3);

	Layer* pcView = GetLayer();

	if ( GetClose() )
	{
		pcView->FillRect( r, sFillColor );
		pcView->DrawFrame( r, FRAME_RECESSED | FRAME_TRANSPARENT );
		pcView->DrawFrame( L, FRAME_RAISED | FRAME_TRANSPARENT );
	}
	else
	{
		pcView->FillRect( r, sFillColor );
		pcView->DrawFrame( r, FRAME_RAISED | FRAME_TRANSPARENT );
		pcView->DrawFrame( L, FRAME_RECESSED | FRAME_TRANSPARENT );
	}
}


void DefaultDecorator::Draw(BRect cUpdateRect)
{
	if(_look==B_NO_BORDER_WINDOW_LOOK)
		return;

	rgb_color sFillColor =  GetFocus() ? GetDefaultColor( PEN_SELWINTITLE ) : GetDefaultColor( PEN_WINTITLE );

	Layer* pcView = GetLayer();

	BRect  cOBounds = pcView->Bounds();
	BRect  cIBounds = cOBounds;

	cIBounds.left   += m_vLeftBorder - 1;
	cIBounds.right  -= m_vRightBorder - 1;
	cIBounds.top    += m_vTopBorder - 1;
	cIBounds.bottom -= m_vBottomBorder - 1;

	pcView->DrawFrame( cOBounds, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
	pcView->DrawFrame( cIBounds, FRAME_RECESSED | FRAME_THIN | FRAME_TRANSPARENT );

	// Bottom
	pcView->FillRect( BRect( cOBounds.left + 1, cIBounds.bottom + 1, cOBounds.right - 1, cOBounds.bottom - 1 ), sFillColor );
	// Left
	pcView->FillRect( BRect( cOBounds.left + 1, cOBounds.top + m_vTopBorder,
							cIBounds.left - 1, cIBounds.bottom ), sFillColor );
	// Right
	pcView->FillRect( BRect( cIBounds.right + 1, cOBounds.top + m_vTopBorder,
							cOBounds.right - 1, cIBounds.bottom ), sFillColor );

	if ( (_flags & B_NOT_CLOSABLE) == 0 )
	{
		_DrawClose(_closerect);
	}

	if(_look!=B_NO_BORDER_WINDOW_LOOK)
	{
		pcView->FillRect( _tabrect, sFillColor );
		pcView->SetHighColor( 255, 255, 255, 0 );
		pcView->SetLowColor( sFillColor );
		pcView->MovePenTo( _tabrect.left + 5,
						(_tabrect.Height()+1.0f) / 2 -
						(m_sFontHeight.ascent + m_sFontHeight.descent) / 2 + m_sFontHeight.ascent +
						m_sFontHeight.leading * 0.5f );

		pcView->DrawString( GetTitle(), -1 );
		pcView->DrawFrame( _tabrect, FRAME_RAISED | FRAME_TRANSPARENT );
	}
	else
	{
		pcView->FillRect( BRect( cOBounds.left + 1, cOBounds.top - 1, cOBounds.right - 1, cIBounds.top + 1 ), sFillColor );
	}

	if ( (_flags & B_NOT_ZOOMABLE) == 0 )
	{
		_DrawZoom(_zoomrect);
	}
}
