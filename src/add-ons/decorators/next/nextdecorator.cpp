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

#include "nextdecorator.h"
#include <Layer.h>

#include <Window.h>
#include <macros.h>



NextDecorator::NextDecorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 nWndFlags ) :
    Decorator( pcLayer, wlook, wfeel, nWndFlags )
{
    m_sFontHeight = pcLayer->GetFontHeight();
  
    CalculateBorderSizes();
}

NextDecorator::~NextDecorator()
{
}

void NextDecorator::CalculateBorderSizes()
{
	if ( _look == B_NO_BORDER_WINDOW_LOOK )
	{
		m_vLeftBorder   = 0;
		m_vRightBorder  = 0;
		m_vTopBorder    = 0;
		m_vBottomBorder = 0;
	}
	else
	{
		if ( (_look == B_TITLED_WINDOW_LOOK)   ||
			 (_look == B_DOCUMENT_WINDOW_LOOK) ||
			 (_look == B_FLOATING_WINDOW_LOOK) )
		{
			m_vTopBorder = float(m_sFontHeight.ascent + m_sFontHeight.descent + 6);
		}
		else
		{
			m_vTopBorder = 4;
		}

		m_vLeftBorder   = 2;
		m_vRightBorder  = 2;

		if ( (_flags & B_NOT_RESIZABLE) != B_NOT_RESIZABLE )
		{
			m_vBottomBorder = 8;
		}
		else
		{
			m_vBottomBorder = 4;
		}
	}
}

void NextDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	_DoLayout();
}

void NextDecorator::SetFlags(uint32 nFlags)
{
	Layer* pcView = GetLayer();
	Decorator::SetFlags(nFlags);
	CalculateBorderSizes();
	pcView->Invalidate();
	_DoLayout();
}

BPoint NextDecorator::GetMinimumSize()
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

click_type NextDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	if ( (_flags & B_NOT_RESIZABLE) != B_NOT_RESIZABLE )
	{
		if ( pt.y < 4 )
		{
			if ( pt.x < 4 )
			{
				return( CLICK_RESIZE_LT );
			}
				else if ( pt.x > _frame.right - 4 )
			{
				return( CLICK_RESIZE_RT );
			}
			else
			{
				return( CLICK_RESIZE_T );
			}
		}
		else if ( pt.y > _frame.bottom - 8 )
		{
			if ( pt.x < 16 )
			{
				return( CLICK_RESIZE_LB );
			}
			else if ( pt.x > _frame.right - 16 )
			{
				return( CLICK_RESIZE_RB );
			}
			else
			{
				return( CLICK_RESIZE_B );
			}
		}
		else if ( pt.x < 2 )
		{
			return( CLICK_RESIZE_L );
		}
		else if ( pt.x > _frame.right - 2 )
		{
			return( CLICK_RESIZE_R );
		}
	}


	if ( _closerect.Contains( pt ) )
	{
		return( DEC_CLOSE );
	}
	else if ( _zoomrect.Contains( pt ) )
	{
		return( DEC_ZOOM );
	}
	else if ( _tabrect.Contains( pt ) )
	{
		return( DEC_DRAG );
	}

	return( DEC_NONE );
}

BRect NextDecorator::GetBorderSize()
{
    return( BRect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void NextDecorator::SetTitle( const char* pzTitle )
{
	Decorator::SetTitle(pzTitle);
	Draw( _frame );
}

void NextDecorator::_SetFocus()
{
	Draw(_frame);
}

void NextDecorator::FrameSized( const BRect& cFrame )
{
	Layer* pcView = GetLayer();
	BPoint cDelta( cFrame.Width() - _frame.Width(), cFrame.Height() - _frame.Height() );
	_frame = cFrame;
	_frame.OffsetTo(0,0);
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

void NextDecorator::_DoLayout()
{
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

	if (!(_flags & B_NOT_ZOOMABLE))
	{
		_zoomrect.right = _frame.right;
		_zoomrect.left   = ceil( _zoomrect.right - m_vTopBorder * 1.5f);
	}

	_zoomrect.top  = 0;
	_zoomrect.bottom  = m_vTopBorder - 1;

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
		_tabrect.right  = _zoomrect.left - 1.0f;
	}
	_tabrect.top  = 0;
	_tabrect.bottom  = m_vTopBorder - 1;
}

void NextDecorator::SetCloseButtonState( bool bPushed )
{
	SetClose(bPushed);
	if ( (_flags & B_NOT_CLOSABLE) == 0 )
	{
		_DrawClose(_closerect);
	}
}

void NextDecorator::SetZoomButtonState( bool bPushed )
{
	SetZoom(bPushed);
	if ( (_flags & B_NOT_ZOOMABLE) == 0 )
	{
		_DrawZoom(_zoomrect);
	}
}


void NextDecorator::_DrawZoom(BRect cRect)
{
	rgb_color sFillColor =  GetFocus() ? GetDefaultColor( PEN_SELWINTITLE ) : GetDefaultColor( PEN_WINTITLE );
	BRect	L,R;

	L.left = cRect.left + ((cRect.Width()+1.0f) / 6);
	L.top = cRect.top + ((cRect.Height()+1.0f) / 6);

	L.right = cRect.right - ((cRect.Width()+1.0f) / 6);
	L.bottom = cRect.bottom - ((cRect.Height()+1.0f) / 6);

	R.left = L.left + 1;
	R.top = L.top + 1;

	R.right = R.left + ((cRect.Width()+1.0f) / 3);
	R.bottom = R.top + ((cRect.Height()+1.0f) / 3);

	Layer* pcView = GetLayer();

	if ( GetZoom() )
	{
		pcView->FillRect( cRect, sFillColor );
		pcView->DrawFrame( cRect, FRAME_RECESSED | FRAME_TRANSPARENT );
		pcView->DrawFrame( R, FRAME_RAISED | FRAME_TRANSPARENT  );
		pcView->DrawFrame( L, FRAME_RECESSED | FRAME_TRANSPARENT );

		pcView->FillRect( BRect( L.left + 1, L.top + 1, L.right - 1, L.bottom - 1 ), sFillColor );
	}
	else
	{
		pcView->FillRect( cRect, sFillColor );
		pcView->DrawFrame( cRect, FRAME_RAISED | FRAME_TRANSPARENT );
		pcView->DrawFrame( L, FRAME_RAISED | FRAME_TRANSPARENT );
		pcView->DrawFrame( R, FRAME_RECESSED | FRAME_TRANSPARENT  );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void NextDecorator::_DrawClose(BRect cRect)
{
	rgb_color sFillColor =  GetFocus() ? GetDefaultColor( PEN_SELWINTITLE ) : GetDefaultColor( PEN_WINTITLE );
	BRect L;

	L.left = cRect.left + ((cRect.Width()+1.0f) / 3);
	L.top = cRect.top + ((cRect.Height()+1.0f) / 3);

	L.right = cRect.right - ((cRect.Width()+1.0f) / 3);
	L.bottom = cRect.bottom - ((cRect.Height()+1.0f) / 3);

	Layer* pcView = GetLayer();

	if ( GetClose() )
	{
		pcView->FillRect( cRect, sFillColor );
		pcView->DrawFrame( cRect, FRAME_RECESSED | FRAME_TRANSPARENT );
		pcView->DrawFrame( L, FRAME_RAISED | FRAME_TRANSPARENT );
	}
	else
	{
		pcView->FillRect( cRect, sFillColor );
		pcView->DrawFrame( cRect, FRAME_RAISED | FRAME_TRANSPARENT );
		pcView->DrawFrame( L, FRAME_RECESSED | FRAME_TRANSPARENT );
	}
}

void NextDecorator::Draw(BRect cUpdateRect )
{
	if ( _look == B_NO_BORDER_WINDOW_LOOK )
		return;

	rgb_color sFillColor =  GetFocus() ? GetDefaultColor( PEN_SELWINTITLE ) : GetDefaultColor( PEN_WINTITLE );

	Layer* pcView = GetLayer();

	BRect  cOBounds = pcView->Bounds();
	BRect  cIBounds = cOBounds;

	cIBounds.left += m_vLeftBorder - 1;
	cIBounds.right -= m_vRightBorder - 1;
	cIBounds.top += m_vTopBorder - 1;
	cIBounds.bottom -= m_vBottomBorder - 1;

	pcView->DrawFrame( cOBounds, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
	pcView->DrawFrame( cIBounds, FRAME_RECESSED | FRAME_THIN | FRAME_TRANSPARENT );
	// Bottom
	pcView->FillRect( BRect( cOBounds.left + 1, cIBounds.bottom + 1, cOBounds.right - 1, cOBounds.bottom - 1 ), sFillColor );

	if ( (_flags & B_NOT_RESIZABLE) != B_NOT_RESIZABLE )
	{
		pcView->SetHighColor( 0, 0, 0, 255 );
		pcView->DrawLine( BPoint( 15, cIBounds.bottom + 1 ), BPoint( 15, cOBounds.bottom - 1 ) );
		pcView->DrawLine( BPoint( cOBounds.right - 16, cIBounds.bottom + 1 ), BPoint( cOBounds.right - 16, cOBounds.bottom - 1 ) );

		pcView->SetHighColor( 255, 255, 255, 255 );
		pcView->DrawLine( BPoint( 16, cIBounds.bottom + 1 ), BPoint( 16, cOBounds.bottom - 1 ) );
		pcView->DrawLine( BPoint( cOBounds.right - 15, cIBounds.bottom + 1 ), BPoint( cOBounds.right - 15, cOBounds.bottom - 1 ) );
	}

	if ( (_flags & B_NOT_CLOSABLE) == 0 )
	{
		_DrawClose(_closerect);
	}

	if (!( _look == B_NO_BORDER_WINDOW_LOOK ))
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

extern "C" float export_get_version()
{
    return( DECORATOR_APIVERSION );
}

extern "C" Decorator* instantiate_decorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 nFlags )
{
    return( new NextDecorator( pcLayer, wlook, wfeel, nFlags ) );
}
