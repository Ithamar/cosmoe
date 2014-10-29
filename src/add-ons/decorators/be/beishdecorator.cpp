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
 * 
 *  Modification of NextDecorator by Edwin de Jonge
 *  BeIsh 0.1
 */

#include "beishdecorator.h"
#include "gradient.h"

#include <Layer.h>
#include "ServerFont.h"
#include <math.h>
#include <Window.h>

static const int CLOSE_BOX_SIZE = 14;
static const int ZOOM_BOX_SIZE = 14;
static const float DRAG_RECT_HEIGHT = 20.0;

BeIshDecorator::BeIshDecorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 nWndFlags ) :
    Decorator( pcLayer, wlook, wfeel, nWndFlags )
{
	m_sFontHeight = pcLayer->GetFontHeight();

	CalculateBorderSizes();
}

BeIshDecorator::~BeIshDecorator()
{
}

void BeIshDecorator::CalculateBorderSizes()
{
	if ( _look == B_NO_BORDER_WINDOW_LOOK )
	{
		m_vLeftBorder   = 0.0f;
		m_vRightBorder  = 0.0f;
		m_vTopBorder    = 0.0f;
		m_vBottomBorder = 0.0f;
	}
	else
	{
		if ( (_look == B_TITLED_WINDOW_LOOK)   ||
			 (_look == B_DOCUMENT_WINDOW_LOOK) ||
			 (_look == B_FLOATING_WINDOW_LOOK) )
		{
			m_vTopBorder = 26.0f;
		}
		else
		{
			m_vTopBorder = 5.0f;
		}

		m_vLeftBorder = 5.0f;
		m_vRightBorder = 5.0f;
		m_vBottomBorder = 5.0f;
	}
}

void BeIshDecorator::FontChanged()
{
	Layer* pcView = GetLayer();
	m_sFontHeight = pcView->GetFontHeight();
	CalculateBorderSizes();
	pcView->Invalidate();
	_DoLayout();
}

void BeIshDecorator::SetFlags( uint32 nFlags )
{
	Layer* pcView = GetLayer();
	Decorator::SetFlags(nFlags);
	CalculateBorderSizes();
	pcView->Invalidate();
	_DoLayout();
}

BPoint BeIshDecorator::GetMinimumSize()
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

click_type BeIshDecorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
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


void BeIshDecorator::DrawGradientRect(const BRect& r,const rgb_color& sHighlight,const rgb_color &sNormal,const rgb_color &sShadow)
{
	Layer *pcView = GetLayer();
	BRect rGradient(r.left + 2.0f, r.top + 2.0f, r.right - 2.0f, r.bottom - 2.0f);
	//int height = floor(rGradient.bottom - rGradient.top);
	int32 height = rGradient.IntegerHeight();
	Gradient gradientHN(sHighlight, sNormal, height);
	Gradient gradientSN(sShadow, sNormal, height);

	for (int i = 0; i < height; i++)
	{
		BPoint left(rGradient.left,rGradient.top + float(i));
		BPoint top(rGradient.left + float(i),rGradient.top);
		pcView->SetHighColor(gradientHN.color[i]);
		pcView->DrawLine(left,top);
		BPoint right(rGradient.right,rGradient.bottom - float(i));
		BPoint bottom(rGradient.right - float(i), rGradient.bottom);
		if (bottom == top)
			continue;
		pcView->SetHighColor(gradientSN.color[i]);
		pcView->DrawLine(right,bottom);
	}

	pcView->SetHighColor(sHighlight);
	pcView->DrawLine(BPoint(r.left + 1,r.top + 1),BPoint(r.left + 1, r.bottom));
	pcView->DrawLine(BPoint(r.left + 2,r.top + 1),BPoint(r.right, r.top + 1));
	pcView->DrawLine(BPoint(r.right,r.top + 1),BPoint(r.right,r.bottom));
	pcView->DrawLine(BPoint(r.left + 1,r.bottom),BPoint(r.right,r.bottom));

	pcView->SetHighColor(sShadow);
	pcView->DrawLine(BPoint(r.left,r.top),BPoint(r.left,r.bottom));
	pcView->DrawLine(BPoint(r.left,r.top),BPoint(r.right,r.top));
	pcView->DrawLine(BPoint(r.right - 1,r.top + 2),BPoint(r.right - 1,r.bottom - 1));
	pcView->DrawLine(BPoint(r.left + 2,r.bottom - 1),BPoint(r.right - 1,r.bottom - 1));
}


BRect BeIshDecorator::GetBorderSize()
{
	return( BRect( m_vLeftBorder, m_vTopBorder, m_vRightBorder, m_vBottomBorder ) );
}

void BeIshDecorator::SetTitle( const char* pzTitle )
{
	Decorator::SetTitle(pzTitle);
	Draw( _frame );
}

void BeIshDecorator::_SetFocus(void)
{
	Draw( _frame );
}

void BeIshDecorator::FrameSized( const BRect& cFrame )
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

		cDamage.top = cDamage.bottom - std::max( m_vBottomBorder, m_vBottomBorder + cDelta.y ) - 1.0f;
		pcView->Invalidate( cDamage );
	}
}

void BeIshDecorator::_DoLayout()
{
	// Bounds in which close box will be drawn.
	_closerect.left = 4;
	_closerect.right = _closerect.left + CLOSE_BOX_SIZE;
	_closerect.top = 4;
	_closerect.bottom = _closerect.top + CLOSE_BOX_SIZE;
	
	// Bounds in which zoom box will be drawn.
	_zoomrect.right = _frame.right - 4 - 1;
	_zoomrect.left  = _zoomrect.right - ZOOM_BOX_SIZE;
	_zoomrect.top  = 4;
	_zoomrect.bottom  = _zoomrect.top + ZOOM_BOX_SIZE;

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
	_tabrect.top  = 1;
	_tabrect.bottom  = _tabrect.top + DRAG_RECT_HEIGHT;
}

void BeIshDecorator::SetCloseButtonState( bool bPushed )
{
	SetClose(bPushed);
	if ( (_flags & B_NOT_CLOSABLE) == 0 )
	{
		_DrawClose(_closerect);
	}
}

void BeIshDecorator::SetZoomButtonState( bool bPushed )
{
	SetZoom(bPushed);
	if ( (_flags & B_NOT_ZOOMABLE) == 0 )
	{
		_DrawZoom( _zoomrect);
	}
}


void BeIshDecorator::_DrawZoom(BRect cRect)
{
	rgb_color sFillColor = ((GetFocus())?_colors->window_tab:_colors->inactive_window_tab).GetColor32();
	Layer* pcView = GetLayer();

	rgb_color sHighlight = get_default_color(COL_SHINE);
	sHighlight.red /= 2;
	sHighlight.green /= 2;
	sHighlight.blue /= 2;
	sHighlight.red += sFillColor.red / 2;
	sHighlight.green += sFillColor.green / 2;
	sHighlight.blue += sFillColor.blue / 2;

	rgb_color sShadow = get_default_color(COL_SHADOW);
	sShadow.red /= 2;
	sShadow.green /= 2;
	sShadow.blue /= 2;
	sShadow.red += sFillColor.red / 2;
	sShadow.green += sFillColor.green / 2;
	sShadow.blue += sFillColor.blue / 2;

	// the small (left) and large (right) rectangles that form the zoom box
	BRect	leftRect, rightRect;

	leftRect.left = cRect.left;
	leftRect.top = cRect.top;

	leftRect.right = leftRect.left + 9.0f;
	leftRect.bottom = leftRect.top + 9.0f;

	rightRect.left = leftRect.left + 3.0f;
	rightRect.top = leftRect.top + 3.0f;

	rightRect.right = rightRect.left + 11.0f;
	rightRect.bottom = rightRect.top + 11.0f;

	if ( GetZoom() )
	{
		pcView->FillRect(rightRect,sFillColor);
		DrawGradientRect(rightRect,sShadow,sFillColor,sHighlight);
		pcView->FillRect(leftRect,sFillColor);
		DrawGradientRect(leftRect,sShadow,sFillColor,sHighlight);
	}
	else
	{
		pcView->FillRect(rightRect,sFillColor);
		DrawGradientRect(rightRect,sHighlight,sFillColor,sShadow);
		pcView->FillRect(leftRect,sFillColor);
		DrawGradientRect(leftRect,sHighlight,sFillColor,sShadow);
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BeIshDecorator::_DrawClose(BRect cRect)
{
	rgb_color sFillColor = ((GetFocus())?_colors->window_tab:_colors->inactive_window_tab).GetColor32();
	Layer* pcView = GetLayer();

	pcView->FillRect(cRect,sFillColor);

	rgb_color sHighlight = get_default_color(COL_SHINE);
	sHighlight.red /= 2;
	sHighlight.green /= 2;
	sHighlight.blue /= 2;
	sHighlight.red += sFillColor.red / 2;
	sHighlight.green += sFillColor.green / 2;
	sHighlight.blue += sFillColor.blue / 2;

	rgb_color sShadow = get_default_color(COL_SHADOW);
	sShadow.red /= 2;
	sShadow.green /= 2;
	sShadow.blue /= 2;
	sShadow.red += sFillColor.red / 2;
	sShadow.green += sFillColor.green / 2;
	sShadow.blue += sFillColor.blue / 2;

	if (GetClose())
	{
		DrawGradientRect(cRect, sShadow, sFillColor, sHighlight);
	}
	else
	{
		DrawGradientRect(cRect, sHighlight, sFillColor, sShadow);
	}
}


void BeIshDecorator::Draw(BRect cUpdateRect)
{
	if ( _look == B_NO_BORDER_WINDOW_LOOK )
		return;

	rgb_color sFillColor = ((GetFocus())?_colors->window_tab:_colors->inactive_window_tab).GetColor32();

	Layer* pcView = GetLayer();

	BRect cOBounds = pcView->Bounds();

	// Draw shadow on bottom and right sides of window.
	// Take off 1 pixel from left of bottom line and top of right line,
	// so that it looks more like a shadow.
	pcView->SetHighColor(get_default_color(COL_SHADOW));
	pcView->DrawLine(BPoint(cOBounds.left, cOBounds.bottom),
					 BPoint(cOBounds.right, cOBounds.bottom));
	pcView->DrawLine(BPoint(cOBounds.right, cOBounds.bottom),
					 BPoint(cOBounds.right, cOBounds.top));

	//remove shadow from bottom and right edges
	cOBounds.right -= 1;
	cOBounds.bottom -= 1;

	/* Begin drawing border around content area. */

	pcView->StrokeRect(cOBounds, get_default_color(COL_NORMAL_WND_BORDER));

	BRect  cIBounds = cOBounds;
	cIBounds.left += m_vLeftBorder - 3;
	cIBounds.right -= m_vRightBorder - 4;
	cIBounds.top += m_vTopBorder - 3;
	cIBounds.bottom -= m_vBottomBorder - 4;

	rgb_color inbetween = get_default_color(COL_NORMAL);
	inbetween.red /= 2;
	inbetween.green /= 2;
	inbetween.blue /= 2;

	rgb_color darker = get_default_color(COL_NORMAL_WND_BORDER);
	darker.red /= 2;
	darker.green /= 2;
	darker.blue /= 2;

	inbetween.red += darker.red;
	inbetween.blue += darker.blue;
	inbetween.green += darker.green;

	pcView->StrokeRect(cIBounds, inbetween);

	pcView->SetHighColor(get_default_color(COL_NORMAL_WND_BORDER));
	pcView->DrawLine(BPoint(cIBounds.left,cIBounds.bottom + 1),
					 BPoint(cIBounds.right+1,cIBounds.bottom + 1));
	pcView->DrawLine(BPoint(cIBounds.right+1,cIBounds.bottom + 1),
					 BPoint(cIBounds.right + 1, cIBounds.top - 1));
	pcView->SetHighColor(get_default_color(COL_SHINE));
	pcView->DrawLine(BPoint(cIBounds.right + 1, cIBounds.top - 1),
					 BPoint(cIBounds.left - 1, cIBounds.top - 1));
	pcView->DrawLine(BPoint(cIBounds.left - 1, cIBounds.top - 1),
					 BPoint(cIBounds.left - 1, cIBounds.bottom));

	//extra line
	pcView->SetHighColor(get_default_color(COL_NORMAL));
	pcView->DrawLine(BPoint(cIBounds.left,cIBounds.top - 1),
					 BPoint(cIBounds.right,cIBounds.top - 1));
	pcView->DrawLine(BPoint(cIBounds.left,cIBounds.top),
					 BPoint(cIBounds.right,cIBounds.top));

	BRect  cIIBounds = cOBounds;
	cIIBounds.left += m_vLeftBorder - 1.0f;
	cIIBounds.right -= m_vRightBorder - 2.0f;
	cIIBounds.top += m_vTopBorder - 1.0f;
	cIIBounds.bottom -= m_vBottomBorder - 2.0f;

	pcView->StrokeRect(cIIBounds,get_default_color(COL_NORMAL_WND_BORDER));

	pcView->SetHighColor(get_default_color(COL_SHINE));
	pcView->DrawLine(BPoint(cIIBounds.left,cIIBounds.bottom + 1),BPoint(cIIBounds.right+1,cIIBounds.bottom + 1));
	pcView->DrawLine(BPoint(cIIBounds.right + 1, cIIBounds.top - 1));
	pcView->SetHighColor(get_default_color(COL_NORMAL_WND_BORDER));
	pcView->DrawLine(BPoint(cIIBounds.left - 1, cIIBounds.top - 1));
	pcView->DrawLine(BPoint(cIIBounds.left - 1, cIIBounds.bottom + 1));

	if ( (_flags & B_NOT_RESIZABLE) == 0 )
	{
		// Draw bottom left and bottom right grooves that indicate window is resizable.
		pcView->SetHighColor( 0, 0, 0, 255 );
		pcView->DrawLine( BPoint( 15, cIBounds.bottom + 1 ), BPoint( 15, cOBounds.bottom - 1 ) );
		pcView->DrawLine( BPoint( cOBounds.right - 16, cIBounds.bottom + 1 ), BPoint( cOBounds.right - 16, cOBounds.bottom - 1 ) );

		pcView->SetHighColor( 255, 255, 255, 255 );
		pcView->DrawLine( BPoint( 16, cIBounds.bottom + 1 ), BPoint( 16, cOBounds.bottom - 1 ) );
		pcView->DrawLine( BPoint( cOBounds.right - 15, cIBounds.bottom + 1 ), BPoint( cOBounds.right - 15, cOBounds.bottom - 1 ) );
	}

	/** End drawing border around content area. **/


	// Draw title bar.
	BRect titleBarRect = _tabrect;
	// Expand to include entire title bar area.
	titleBarRect.left = 1;
	// subtract 2 from _frame.right to avoid drawing over window outline and shadow.
	titleBarRect.right = _frame.right - 2;
	pcView->FillRect( titleBarRect, sFillColor );

	rgb_color sHighlight = get_default_color(COL_SHINE);
	sHighlight.red /= 2;
	sHighlight.green /= 2;
	sHighlight.blue /= 2;
	sHighlight.red += sFillColor.red / 2;
	sHighlight.green += sFillColor.green / 2;
	sHighlight.blue += sFillColor.blue / 2;

	pcView->SetHighColor(sHighlight);
	pcView->DrawLine(titleBarRect.LeftBottom(),titleBarRect.LeftTop());
	pcView->DrawLine(titleBarRect.LeftTop(),titleBarRect.RightTop());

	// Draw title, if allowed.
	if (!( _look == B_NO_BORDER_WINDOW_LOOK ))
	{
		pcView->SetHighColor(0, 0, 0, 0);
		pcView->SetLowColor( sFillColor );
		pcView->MovePenTo( _tabrect.left + 17,
				(_tabrect.Height()+1.0f) / 2 -
				(m_sFontHeight.ascent + m_sFontHeight.descent) / 2 + m_sFontHeight.ascent +
				m_sFontHeight.leading * 0.5f + 1);

	//	const char *title = m_cTitle.c_str();
	//	int titlewidth = pcView->m_pcFont->GetInstance()->GetStringWidth(title,strlen(title));
	//	printf("titlewidth = %d",titlewidth);
		pcView->DrawString( GetTitle(), -1 );
	}

	if ( (_flags & B_NOT_CLOSABLE) == 0 )
	{
		_DrawClose( _closerect);
	}
	if ( (_flags & B_NOT_ZOOMABLE) == 0 )
	{
		_DrawZoom( _zoomrect);
	}
}

extern "C" float export_get_version(void)
{
	return( DECORATOR_APIVERSION );
}

extern "C" Decorator* instantiate_decorator( Layer* pcLayer, int32 wlook, int32 wfeel, uint32 nFlags )
{
	return( new BeIshDecorator( pcLayer, wlook, wfeel, nFlags ) );
}
