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
//	File Name:		WinBorder.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//	Description:	Layer subclass which handles window management
//  
//------------------------------------------------------------------------------
#include "ServerWindow.h"
#include "Decorator.h"
#include "WinBorder.h"
#include "AppServer.h"	// for new_decorator()
#include "CursorManager.h"

#include "ServerFont.h"
#include "sprite.h"
#include "ServerApp.h"

#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <Window.h>

static int Align( int nVal, int nAlign );
static int Clamp( int nVal, int nDelta, int nMin, int nMax );


// Toggle general function call output
//#define DEBUG_WINBORDER

// toggle
//#define DEBUG_WINBORDER_MOUSE
//#define DEBUG_WINBORDER_CLICK

#ifdef DEBUG_WINBORDER
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#ifdef DEBUG_WINBORDER_MOUSE
#	include <stdio.h>
#	define STRACE_MOUSE(x) printf x
#else
#	define STRACE_MOUSE(x) ;
#endif

#ifdef DEBUG_WINBORDER_CLICK
#	include <stdio.h>
#	define STRACE_CLICK(x) printf x
#else
#	define STRACE_CLICK(x) ;
#endif



static inline BRect RectToClient( const BRect& cRect, Decorator* pcDecorator )
{
	BRect cBorders = pcDecorator->GetBorderSize();

	return(  BRect( cRect.left   + cBorders.left,
					cRect.top    + cBorders.top,
					cRect.right  - cBorders.right,
					cRect.bottom - cBorders.bottom ) );
}


WinBorder::WinBorder( ServerWindow* pcWindow, Layer* pcParent, const char* pzName, bool bBackdrop )
    : Layer( BRect(0,0,0,0), pzName, pcParent, 0, NULL, pcWindow ), m_cMinSize(0,0),m_cMaxSize(INT_MAX,INT_MAX),
      m_cAlignSize(1,1), m_cAlignSizeOff(0,0), m_cAlignPos(1,1), m_cAlignPosOff(0,0)
{
	m_eHitItem = DEC_NONE;
	m_eCursorHitItem = DEC_NONE;

	m_nZoomDown   = 0;
	m_nCloseDown  = 0;

	m_bWndMovePending = false;

	m_bBackdrop = bBackdrop;

//    FontNode* pcNode = new FontNode( AppServer::GetInstance()->GetPlainFont() );
//    SetFont( pcNode );
	SetFont(app_server->GetWindowTitleFont() );

	fDecorator = NULL;

	fTopLayer = new Layer( BRect(0,0,0,0), "wnd_client", this, 0, NULL, pcWindow );
	
	printf("%s: fTopLayer is %p\n", pzName, fTopLayer);
	fflush(stdout);
}

/*!
	\brief Handles B_MOUSE_DOWN events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_DOWN message
	\param sendMessage flag to send a B_MOUSE_DOWN message to the client
	
	This function searches to see if the B_MOUSE_DOWN message is being sent to the window tab
	or frame. If it is not, the message is passed on to the appropriate view in the client
	BWindow. If the WinBorder is the target, then the proper action flag is set.
*/
bool WinBorder::MouseDown( BMessenger* pcAppTarget, const BPoint& cPos, int nButton )
{
	IPoint cHitPos;

	m_cDeltaMove = IPoint( 0, 0 );
	m_cDeltaSize = IPoint( 0, 0 );
	m_cRawFrame = m_cIFrame;

	if ( Bounds().Contains( cPos ) && fTopLayer->fFrame.Contains( cPos ) == false )
	{
		m_eHitItem = fDecorator->Clicked(cPos, 0, 0);
	}
	else
	{
		m_eHitItem = DEC_NONE;
	}

	cHitPos = GetHitPointBase();

	m_cHitOffset = IPoint(cPos) + GetILeftTop() - m_cRawFrame.LeftTop() - cHitPos;

	if ( m_eHitItem == DEC_CLOSE )
	{
		m_nCloseDown = 1;
		fDecorator->SetCloseButtonState( true );
	}
	else if ( m_eHitItem == DEC_ZOOM )
	{
		m_nZoomDown = 1;
		fDecorator->SetZoomButtonState( true );
	}
	else if ( m_eHitItem == DEC_DRAG )
	{
		cursormanager->SetCursor(B_CURSOR_DRAG);
	}
	return( m_eHitItem != DEC_NONE );
}

/*!
	\brief Handles B_MOUSE_MOVED events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_MOVED message
	
	This function doesn't do much except test continue any move/resize operations in progress 
	or check to see if the user clicked on a tab button (close, zoom, etc.) and then moused
	away to prevent the operation from occurring
*/
bool WinBorder::MouseMoved( BMessenger* pcAppTarget, const BPoint& cNewPos, int nTransit )
{
	IPoint cHitPos;

	if ( m_eHitItem == DEC_NONE && nTransit == B_EXITED_VIEW )
	{
		m_eCursorHitItem = DEC_NONE;
		return(false);
	}

	// Caclculate the window borders
	IRect cBorders(0,0,0,0);
	int nBorderWidth  = 0;
	int nBorderHeight = 0;
	if ( fDecorator != NULL )
	{
		cBorders = static_cast<IRect>(fDecorator->GetBorderSize());
		nBorderWidth  = cBorders.left + cBorders.right;
		nBorderHeight = cBorders.top + cBorders.bottom;
	}

	// Figure out which edges the cursor is relative to
	cHitPos = GetHitPointBase();

	// Calculate the delta movement relative to the hit edge/corner
	IPoint cDelta( IPoint(cNewPos) - (cHitPos + m_cHitOffset) - m_cDeltaMove + GetILeftTop() -m_cRawFrame.LeftTop() );

	click_type eHitItem;
	if ( Bounds().Contains( cNewPos ) && fTopLayer->fFrame.Contains( cNewPos ) == false ) {
		eHitItem = fDecorator->Clicked(cNewPos, 0, 0);
	}
	else
	{
		eHitItem = DEC_NONE;
	}

	if ( m_eHitItem == DEC_NONE && (/*InputNode::GetMouseButtons() == 0||*/ fServerWin->HasFocus() ) )
	{
		// Change the mouse-pointer to indicate that resizeing is possible if above one of the resize-edges
		click_type eCursorHitItem = (m_eHitItem == DEC_NONE) ? eHitItem : m_eHitItem;
		if ( eCursorHitItem != m_eCursorHitItem )
		{
			m_eCursorHitItem = eCursorHitItem;
			switch( eCursorHitItem )
			{
				case CLICK_RESIZE_L:
				case CLICK_RESIZE_R:
					cursormanager->SetCursor(B_CURSOR_RESIZE_EW);
					break;
				case CLICK_RESIZE_T:
				case CLICK_RESIZE_B:
					cursormanager->SetCursor(B_CURSOR_RESIZE_NS);
					break;
				case CLICK_RESIZE_LT:
				case CLICK_RESIZE_RB:
					cursormanager->SetCursor(B_CURSOR_RESIZE_NWSE);
					break;
				case CLICK_RESIZE_LB:
				case CLICK_RESIZE_RT:
					cursormanager->SetCursor(B_CURSOR_RESIZE_NESW);
					break;
				case DEC_DRAG:
					cursormanager->SetCursor(B_CURSOR_DRAG);
					break;
				default:
					cursormanager->SetCursor(B_CURSOR_DEFAULT);
					break;
			}
		}

		// If we didnt hit anything interesting with the last mouse-click we are done by now.
		if ( m_eHitItem == DEC_NONE )
		{
			return(false);
		}
	}
	// Set the state of the various border buttons.
	if ( m_nZoomDown > 0 )
	{
		int nNewMode = (eHitItem == DEC_ZOOM) ? 1 : 2;
		if ( nNewMode != m_nZoomDown )
		{
			m_nZoomDown = nNewMode;
			fDecorator->SetZoomButtonState( m_nZoomDown == 1 );
		}
	}
	else if ( m_nCloseDown > 0 )
	{
		int nNewMode = (eHitItem == DEC_CLOSE) ? 1 : 2;
		if ( nNewMode != m_nCloseDown )
		{
			m_nCloseDown = nNewMode;
			fDecorator->SetCloseButtonState( m_nCloseDown == 1 );
		}
	}
	else if ( m_eHitItem == DEC_DRAG )
	{
		cDelta.x = Align( cDelta.x, m_cAlignPos.x );
		cDelta.y = Align( cDelta.y, m_cAlignPos.y );
		m_cDeltaMove += cDelta;
	}

	uint32 nFlags = fServerWin->Flags();
	if ( (nFlags & B_NOT_RESIZABLE) != B_NOT_RESIZABLE )
	{
		if ( nFlags & B_NOT_H_RESIZABLE )
		{
			cDelta.x = 0;
		}
		if ( nFlags & B_NOT_V_RESIZABLE )
		{
			cDelta.y = 0;
		}
		IPoint cBorderMinSize( fDecorator->GetMinimumSize() );
		IPoint cMinSize( m_cMinSize );

		if ( cMinSize.x < cBorderMinSize.x - nBorderWidth )
		{
			cMinSize.x = cBorderMinSize.x - nBorderWidth;
		}
		if ( cMinSize.y < cBorderMinSize.y - nBorderHeight )
		{
			cMinSize.y = cBorderMinSize.y - nBorderHeight;
		}

		switch( m_eHitItem )
		{
			case CLICK_RESIZE_L:
				cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
				cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
				m_cDeltaMove.x += cDelta.x;
				m_cDeltaSize.x -= cDelta.x;
				break;
			case CLICK_RESIZE_R:
				cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
				cDelta.x = Align( cDelta.x, m_cAlignSize.x );
				m_cDeltaSize.x += cDelta.x;
				break;
			case CLICK_RESIZE_T:
				cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
				cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
				m_cDeltaMove.y += cDelta.y;
				m_cDeltaSize.y -= cDelta.y;
				break;
			case CLICK_RESIZE_B:
				cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
				cDelta.y = Align( cDelta.y, m_cAlignSize.y );
				m_cDeltaSize.y += cDelta.y;
				break;
			case CLICK_RESIZE_LT:
				cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
				cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
				cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
				cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
				m_cDeltaMove += cDelta;
				m_cDeltaSize -= cDelta;
				break;
			case CLICK_RESIZE_RT:
				cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
				cDelta.y = -Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, -cDelta.y, cMinSize.y, m_cMaxSize.y );
				cDelta.x = Align( cDelta.x, m_cAlignSize.x );
				cDelta.y = Align( Align( cDelta.y, m_cAlignSize.y ), m_cAlignPos.y );
				m_cDeltaSize.x += cDelta.x;
				m_cDeltaSize.y -= cDelta.y;
				m_cDeltaMove.y += cDelta.y;
				break;
			case CLICK_RESIZE_RB:
				cDelta.x = Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, cDelta.x, cMinSize.x, m_cMaxSize.x );
				cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
				cDelta.x = Align( cDelta.x, m_cAlignSize.x );
				cDelta.y = Align( cDelta.y, m_cAlignSize.y );
				m_cDeltaSize += cDelta;
				break;
			case CLICK_RESIZE_LB:
				cDelta.x = -Clamp( m_cRawFrame.Width() + m_cDeltaSize.x - nBorderWidth, -cDelta.x, cMinSize.x, m_cMaxSize.x );
				cDelta.y = Clamp( m_cRawFrame.Height() + m_cDeltaSize.y - nBorderHeight, cDelta.y, cMinSize.y, m_cMaxSize.y );
				cDelta.x = Align( Align( cDelta.x, m_cAlignSize.x ), m_cAlignPos.x );
				cDelta.y = Align( cDelta.y, m_cAlignSize.y );
				m_cDeltaMove.x += cDelta.x;
				m_cDeltaSize.x -= cDelta.x;
				m_cDeltaSize.y += cDelta.y;
				break;
			default:
				break;
		}
	}

	if ( pcAppTarget != NULL &&
		(m_cDeltaSize.x != 0 || m_cDeltaSize.y != 0 || m_cDeltaMove.x != 0 || m_cDeltaMove.y != 0 ) )
	{
		m_cRawFrame.right  += m_cDeltaSize.x + m_cDeltaMove.x;
		m_cRawFrame.bottom += m_cDeltaSize.y + m_cDeltaMove.y;
		m_cRawFrame.left   += m_cDeltaMove.x;
		m_cRawFrame.top    += m_cDeltaMove.y;

		IRect cAlignedFrame = AlignRect( m_cRawFrame, cBorders );


		if ( m_bWndMovePending == false )
		{
			BMessage cMsg( M_WINDOW_FRAME_CHANGED );

			cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame.AsBRect(), fDecorator ) );

			if ( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				printf( "WinBorder::MouseMoved() failed to send M_WINDOW_FRAME_CHANGED message to window\n" );
			}
			m_bFrameUpdated = false;
//            m_cClientDeltaSize = IPoint( 0, 0 );
		}
		else
		{
			m_bFrameUpdated = true;
//            m_cClientDeltaSize += m_cDeltaSize;
		}
		DoSetFrame( cAlignedFrame.AsBRect() );
		SrvSprite::Hide();
		fParent->UpdateRegions( false );
		SrvSprite::Unhide();

		m_bWndMovePending = true;

		m_cDeltaMove = IPoint( 0, 0 );
		m_cDeltaSize = IPoint( 0, 0 );
	}
	return( m_eHitItem != DEC_NONE );
}

/*!
	\brief Handles B_MOUSE_UP events and takes appropriate actions
	\param evt PointerEvent object containing the info from the last B_MOUSE_UP message
	
	This function resets any state objects (is_resizing flag and such) and if resetting a 
	button click flag, takes the appropriate action (i.e. clearing the close button flag also
	takes steps to close the window).
*/
void WinBorder::MouseUp( BMessenger* pcAppTarget, const BPoint& cPos, int nButton )
{
	if ( m_nCloseDown == 1 )
	{
		if ( pcAppTarget != NULL )
		{
			BMessage cMsg( B_QUIT_REQUESTED );
			if ( pcAppTarget->SendMessage( &cMsg ) < 0 )
			{
				printf( "Error: WinBorder::MouseUp() failed to send B_QUIT_REQUESTED to window\n" );
			}
		}
	}

	m_eHitItem = DEC_NONE;

	if ( m_nZoomDown == 1 )
		fDecorator->SetZoomButtonState( false );

	if ( m_nCloseDown == 1 )
		fDecorator->SetCloseButtonState( false );

	m_nZoomDown   = 0;
	m_nCloseDown  = 0;
	m_cRawFrame = m_cIFrame;
}

void WinBorder::SetDecorator( Decorator* pcDecorator )
{
	delete fDecorator;
	fDecorator = pcDecorator;
}


void WinBorder::SetFlags( uint32 nFlags )
{
	fDecorator->SetFlags( nFlags );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinBorder::DoSetFrame( const BRect& cRect )
{
	Layer::SetFrame( cRect );
	fTopLayer->SetFrame( RectToClient( Bounds(), fDecorator ) );
	fDecorator->FrameSized( Bounds() );
}

void WinBorder::SetFrame( const BRect& cRect )
{
	m_cRawFrame = cRect;
	DoSetFrame( cRect );
}

void WinBorder::SetSizeLimits( const BPoint& cMinSize, const BPoint& cMaxSize )
{
	m_cMinSize = IPoint(cMinSize);
	m_cMaxSize = IPoint(cMaxSize);
}

void WinBorder::GetSizeLimits( BPoint* pcMinSize, BPoint* pcMaxSize )
{
	*pcMinSize = m_cMinSize.AsBPoint();
	*pcMaxSize = m_cMaxSize.AsBPoint();
}

void WinBorder::SetAlignment( const IPoint& cSize, const IPoint& cSizeOffset, const IPoint& cPos, const IPoint& cPosOffset )
{
	m_cAlignSize    = cSize;
	m_cAlignPos     = cPos;

	m_cAlignSizeOff.x = cSizeOffset.x % cSize.x;
	m_cAlignSizeOff.y = cSizeOffset.y % cSize.y;

	m_cAlignPosOff.x  = cPosOffset.x % cPos.x;
	m_cAlignPosOff.y  = cPosOffset.y % cPos.y;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void WinBorder::RequestDraw( const IRect& cUpdateRect, bool bUpdate )
{
	if ( bUpdate )
	{
		BeginUpdate();
	}

	fDecorator->Draw( const_cast<IRect&>(cUpdateRect).AsBRect() );

	if ( bUpdate )
	{
		EndUpdate();
	}
}


IRect WinBorder::AlignRect( const IRect& cRect, const IRect& cBorders )
{
	IRect cAFrame;

	int nBorderWidth  = cBorders.left + cBorders.right;
	int nBorderHeight = cBorders.top + cBorders.bottom;

	cAFrame.left = ((cRect.left + cBorders.left) / m_cAlignPos.x) * m_cAlignPos.x + m_cAlignPosOff.x - cBorders.left;
	cAFrame.top  = ((cRect.top  + cBorders.top) / m_cAlignPos.y) * m_cAlignPos.y + m_cAlignPosOff.y - cBorders.top;

	cAFrame.right  = cAFrame.left + ((cRect.Width() - nBorderWidth) / m_cAlignSize.x) * m_cAlignSize.x +
		m_cAlignSizeOff.x + nBorderWidth;
	cAFrame.bottom = cAFrame.top  + ((cRect.Height() - nBorderHeight) / m_cAlignSize.y) * m_cAlignSize.y +
		m_cAlignSizeOff.y + nBorderHeight;
	return( cAFrame );
}


static int Align( int nVal, int nAlign )
{
    return( (nVal/nAlign)*nAlign );
}


static int Clamp( int nVal, int nDelta, int nMin, int nMax )
{
	if ( nVal + nDelta < nMin )
	{
		return( nMin - nVal );
	}
	else if ( nVal + nDelta > nMax )
	{
		return( nMax - nVal );
	}
	else
	{
		return( nDelta );
	}
}


IPoint WinBorder::GetHitPointBase() const
{
	switch( m_eHitItem )
	{
		case CLICK_RESIZE_L:
			return( IPoint( 0, 0 ) );
			break;
		case CLICK_RESIZE_R:
			return( IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, 0 ) );
			break;
		case CLICK_RESIZE_T:
			return( IPoint( 0, 0 ) );
			break;
		case CLICK_RESIZE_B:
			return( IPoint( 0, m_cRawFrame.Height() + m_cDeltaSize.y ) );
			break;
		case CLICK_RESIZE_LT:
			return( IPoint( 0, 0 ) );
			break;
		case CLICK_RESIZE_RT:
			return( IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, 0 ) );
			break;
		case CLICK_RESIZE_RB:
			return( IPoint( m_cRawFrame.Width() + m_cDeltaSize.x, m_cRawFrame.Height() + m_cDeltaSize.y ) );
			break;
		case CLICK_RESIZE_LB:
			return( IPoint( 0, m_cRawFrame.Height() + m_cDeltaSize.y ) );
			break;
		default:
			return( IPoint( 0, 0 ) );
			break;
	}
}



void WinBorder::WndMoveReply( BMessenger* pcAppTarget )
{
	g_cLayerGate.Close();
//    if ( m_cDeltaSize.x != 0 || m_cDeltaSize.y != 0 || m_cDeltaMove.x != 0 || m_cDeltaMove.y != 0 )
	if ( m_bFrameUpdated )
	{
		m_bFrameUpdated = false;
		if ( pcAppTarget != NULL )
		{
			IRect cBorders(0,0,0,0);
			if ( fDecorator != NULL )
			{
				cBorders = static_cast<IRect>(fDecorator->GetBorderSize());
			}
/*
			m_cRawFrame.right  += m_cDeltaSize.x + m_cDeltaMove.x;
			m_cRawFrame.bottom += m_cDeltaSize.y + m_cDeltaMove.y;
			m_cRawFrame.left   += m_cDeltaMove.x;
			m_cRawFrame.top    += m_cDeltaMove.y;
			*/
			IRect cAlignedFrame = AlignRect( m_cRawFrame, cBorders );

//            DoSetFrame( cAlignedFrame );
//            SrvSprite::Hide();
//            fParent->UpdateRegions( false );
//            SrvSprite::Unhide();

			BMessage cMsg( M_WINDOW_FRAME_CHANGED );

			cMsg.AddRect( "_new_frame", RectToClient( cAlignedFrame.AsBRect(), fDecorator ) );

			if ( pcAppTarget->SendMessage( &cMsg ) < 0 ) {
				printf( "Error: WinBorder::WndMoveReply() failed to send M_WINDOW_FRAME_CHANGED to window\n" );
			}

//            if ( m_cClientDeltaSize.x > 0 ) {
//                fTopLayer
//            }

//            m_cClientDeltaSize = IPoint( 0, 0 );

			m_cDeltaMove = IPoint( 0, 0 );
			m_cDeltaSize = IPoint( 0, 0 );
			m_bWndMovePending = true;
		}
	}
	else
	{
		m_bWndMovePending = false;
	}

	if ( m_bWndMovePending == false )
		fTopLayer->UpdateIfNeeded( false );

	g_cLayerGate.Open();
}

bool WinBorder::HasPendingSizeEvents() const
{
	return( m_bWndMovePending );
}
