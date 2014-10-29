/*
 *  The Cosmoe application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
 *  Copyright (C) 2002 Bill Hayden
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

#include <stdio.h>

#include "sprite.h"
#include <ServerBitmap.h>
#include "DisplayDriver.h"

#include <Region.h>

#include <algorithm>

BLocker                 SrvSprite::s_cLock( "sprite_lock" );
std::vector<SrvSprite*> SrvSprite::s_cInstances;
int32                   SrvSprite::s_nHideCount = 0;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvSprite::SrvSprite( const IRect& cBounds, const IPoint& cPos, const IPoint& cHotSpot,
                      ServerBitmap* pcTarget, ServerBitmap* pcImage )
{
	s_cLock.Lock();
	m_pcImage          = pcImage;
	m_pcTarget         = pcTarget;
	if ( pcImage != NULL )
	{
		m_pcBackground = new UtilityBitmap( pcImage->Bounds(), pcTarget->ColorSpace(), 0 );
	}
	else
	{
		m_pcBackground = NULL;
	}
	
	m_cPosition = cPos;
	m_cHotSpot  = cHotSpot;
	m_bVisible  = false;

	if ( pcImage != NULL )
	{
		m_cBounds = pcImage->Bounds();
	}
	else
	{
		m_cBounds = cBounds;
	}

	if ( m_pcBackground && m_pcImage )
	{
		Capture( pcTarget, cPos + cHotSpot );
	}
	
	if ( pcTarget->Bits() != NULL )
	{
		Draw();
	}
	
	s_cInstances.push_back( this );
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

SrvSprite::~SrvSprite()
{
	s_cLock.Lock();
	Erase();
	delete m_pcTarget;
	if ( NULL != m_pcBackground )
	{
		delete m_pcBackground;
	}

	if ( NULL != m_pcImage )
	{
		delete m_pcImage;
	}

	std::vector<SrvSprite*>::iterator i = find( s_cInstances.begin(), s_cInstances.end(), this );
	s_cInstances.erase( i );
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------


void SrvSprite::ColorSpaceChanged()
{
	for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
	{
		SrvSprite* pcSprite = s_cInstances[i];
		if ( pcSprite->m_pcImage != NULL )
		{
			delete pcSprite->m_pcBackground;
			pcSprite->m_pcBackground = new UtilityBitmap( pcSprite->m_pcImage->Bounds(),
													pcSprite->m_pcTarget->ColorSpace(), 0 );
		}
	}
}

void SrvSprite::Hide( const IRect& cFrame )
{
	bool        bDoHide = false;
	s_cLock.Lock();
	atomic_add( &s_nHideCount, 1 );

	for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
	{
		SrvSprite* pcSprite = s_cInstances[i];
		if ( cFrame.Contains( pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot ) )
		{
			bDoHide = true;
			break;
		}
	}
	
	if ( bDoHide )
	{
		for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
		{
			s_cInstances[i]->Erase();
		}
	}
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Hide()
{
	s_cLock.Lock();
	if ( atomic_add( &s_nHideCount, 1 ) != 0 )
	{
		s_cLock.Unlock();
		return;
	}

	for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
	{
		SrvSprite* pcSprite = s_cInstances[i];
		if ( pcSprite->m_bVisible )
		{
			pcSprite->Erase();
		}
	}
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Unhide()
{
	s_cLock.Lock();
	if ( atomic_add( &s_nHideCount, -1 ) != 1 )
	{
		s_cLock.Unlock();
		return;
	}

	for ( int i = s_cInstances.size() - 1 ; i >= 0  ; --i )
	{
		SrvSprite* pcSprite = s_cInstances[i];
		if ( pcSprite->m_bVisible == false )
		{
			pcSprite->Draw();
		}
	}
	s_cLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Draw()
{
	if ( m_bVisible )
	{
		return;
	}
	m_bVisible = true;
	DisplayDriver* pcDriver = m_pcTarget->m_pcDriver;

	BRect cBitmapRect = m_pcTarget->Bounds();
	if ( m_pcImage != NULL )
	{
		IRect   cRect     = m_cBounds + m_cPosition - m_cHotSpot;
		IRect   cClipRect = cRect & cBitmapRect;
		IPoint cOffset   = cClipRect.LeftTop() - cRect.LeftTop();

		if ( cClipRect.IsValid() )
		{
#if 0
			pcDriver->BltBitmap( m_pcBackground, m_pcTarget, cClipRect,
								cOffset, B_OP_COPY );
			pcDriver->BltBitmap( m_pcTarget, m_pcImage, cClipRect.Bounds() + cOffset,
								m_cPosition - m_cHotSpot + cOffset, B_OP_BLEND );
#endif
		}
	}
#if 0
	else
	{
		IRect cFrame = m_cBounds + m_cPosition - m_cHotSpot;
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.top ), BPoint( cFrame.right, cFrame.top ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.top ), BPoint( cFrame.right, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.bottom ), BPoint( cFrame.left, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.bottom ), BPoint( cFrame.left, cFrame.top ),
							rgb_color(), B_OP_INVERT );
	}
#endif
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Erase()
{
	if ( m_bVisible == false )
	{
		return;
	}
	DisplayDriver* pcDriver = m_pcTarget->m_pcDriver;

	BRect cBitmapRect = m_pcTarget->Bounds();
	if ( m_pcImage != NULL )
	{
		IRect   cRect     = m_cBounds + m_cPosition - m_cHotSpot;
		IRect   cClipRect = cRect & cBitmapRect;
		IPoint cOffset   = cClipRect.LeftTop() - cRect.LeftTop();

		if ( cClipRect.IsValid() )
		{
#if 0
			pcDriver->BltBitmap( m_pcTarget, m_pcBackground, cClipRect.Bounds() + cOffset,
								m_cPosition - m_cHotSpot + cOffset, B_OP_COPY );
#endif
		}
	}
#if 0
	else
	{
		IRect cFrame = m_cBounds + m_cPosition - m_cHotSpot;
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.top ), BPoint( cFrame.right, cFrame.top ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.top ), BPoint( cFrame.right, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.bottom ), BPoint( cFrame.left, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( m_pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.bottom ), BPoint( cFrame.left, cFrame.top ),
							rgb_color(), B_OP_INVERT );
	}
#endif
	m_bVisible = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Draw( ServerBitmap* pcTarget, const IPoint& cPos )
{
	DisplayDriver* pcDriver = m_pcTarget->m_pcDriver;

	BRect cBitmapRect = pcTarget->Bounds();
	if ( m_pcImage != NULL )
	{

		IRect   cRect     = m_cBounds + cPos;
		IRect   cClipRect = cRect & cBitmapRect;

		if ( cClipRect.IsValid() )
		{
#if 0
			IPoint cOffset   = cClipRect.LeftTop() - cRect.LeftTop();

			pcDriver->BltBitmap( pcTarget, m_pcImage, cClipRect.Bounds() + cOffset,
								cPos + cOffset, B_OP_BLEND );
#endif
		}
	}
#if 0
	else
	{
		IRect cFrame = m_cBounds + cPos;
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.top ), BPoint( cFrame.right, cFrame.top ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.top ), BPoint( cFrame.right, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.bottom ), BPoint( cFrame.left, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.bottom ), BPoint( cFrame.left, cFrame.top ),
							rgb_color(), B_OP_INVERT );
	}
#endif
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Capture( ServerBitmap* pcTarget, const IPoint& cPos )
{
	if ( m_pcImage != NULL && m_pcBackground != NULL )
	{
		DisplayDriver* pcDriver = m_pcTarget->m_pcDriver;

		BRect cBitmapRect = pcTarget->Bounds();
		IRect   cRect     = m_cBounds + cPos;
		IRect   cClipRect = cRect & cBitmapRect;
		IPoint  cOffset   = cClipRect.LeftTop() - cRect.LeftTop();

		if ( cClipRect.IsValid() )
		{
#if 0
			pcDriver->BltBitmap( m_pcBackground, pcTarget, cClipRect,
								cOffset, B_OP_COPY );
#endif
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::Erase( ServerBitmap* pcTarget, const IPoint& cPos )
{
	DisplayDriver* pcDriver = m_pcTarget->m_pcDriver;

	BRect cBitmapRect = pcTarget->Bounds();
	if ( m_pcImage != NULL )
	{
		IRect   cRect     = m_cBounds + cPos;
		IRect   cClipRect = cRect & cBitmapRect;

		if ( cClipRect.IsValid() )
		{
#if 0
			IPoint cOffset   = cClipRect.LeftTop() - cRect.LeftTop();
			pcDriver->BltBitmap( pcTarget, m_pcBackground, cClipRect.Bounds() + cOffset,
								cPos + cOffset, B_OP_COPY );
#endif
		}
	}
#if 0
	else
	{
		IRect cFrame = m_cBounds + cPos;
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.top ), BPoint( cFrame.right, cFrame.top ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.top ), BPoint( cFrame.right, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.right, cFrame.bottom ), BPoint( cFrame.left, cFrame.bottom ),
							rgb_color(), B_OP_INVERT );
		pcDriver->StrokeLine( pcTarget, cBitmapRect,
							BPoint( cFrame.left, cFrame.bottom ), BPoint( cFrame.left, cFrame.top ),
							rgb_color(), B_OP_INVERT );
	}
#endif
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void SrvSprite::MoveBy( const IPoint& cDelta  )
{
	if ( this != s_cInstances[0] )
	{
		return;
	}

	s_cLock.Lock();
	if ( s_nHideCount > 0 )
	{
		for ( int i = s_cInstances.size() - 1 ; i >= 0 ; --i )
		{
			SrvSprite* pcSprite = s_cInstances[i];
			pcSprite->m_cPosition += cDelta;
		}
		s_cLock.Unlock();
		return;
	}

	DisplayDriver* pcDriver = m_pcTarget->m_pcDriver;

	IRect   cOldRect(100000,100000,-100000,-100000);
	IRect   cNewRect(100000,100000,-100000,-100000);

	for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
	{
		SrvSprite* pcSprite = s_cInstances[i];

		if ( pcSprite->m_pcImage == NULL )
		{
			pcSprite->Erase();
			continue;
		}
		IRect cRect( pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot );
		cOldRect |= cRect;
		cNewRect |= cRect + cDelta;
	}

	if ( cOldRect.Contains( cNewRect ) == false )
	{
		for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
		{
			SrvSprite* pcSprite = s_cInstances[i];

			if ( pcSprite->m_pcImage != NULL )
			{
				pcSprite->Erase();
				continue;
			}
		}
		for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
		{
			s_cInstances[i]->m_cPosition += cDelta;
			s_cInstances[i]->Draw();
		}
	}
	else
	{
		BRect   cBitmapRect = m_pcTarget->Bounds();

		IRect cFullRect = cOldRect | cNewRect;
		BRect cTotRect = cFullRect.AsBRect() & cBitmapRect;
		IPoint cLeftTop = IPoint(cTotRect.LeftTop());

		BRegion cReg(cTotRect);

		ServerBitmap* pcTmp = new UtilityBitmap( cTotRect.OffsetToCopy(0,0), m_pcTarget->ColorSpace(), 0 );
		
		for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
		{
			SrvSprite* pcSprite = s_cInstances[i];

			if ( pcSprite->m_pcImage == NULL )
			{
				continue;
			}
			IRect cRect = pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot;
			cReg.Exclude( cRect.AsBRect() );
			pcSprite->Erase( pcTmp, cRect.LeftTop() - cLeftTop );
		}

		BRect aRect;
#if 0
		ENUMBRECTS( cReg, aRect )
		{
			IPoint offset(aRect.LeftTop());
			offset -= cLeftTop;
			pcDriver->BltBitmap( pcTmp, m_pcTarget, aRect, offset, B_OP_COPY );
		}
#endif
		for ( uint i = 0 ; i < s_cInstances.size() ; ++i )
		{
			SrvSprite* pcSprite = s_cInstances[i];

			if ( pcSprite->m_pcImage == NULL )
			{
				continue;
			}
			IRect cRect = pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot;
			IPoint cClipOff  = cRect.LeftTop() - cLeftTop;

			pcSprite->Capture( pcTmp, cClipOff + cDelta );
		}

		for ( int i = s_cInstances.size() - 1 ; i >= 0 ; --i )
		{
			SrvSprite* pcSprite = s_cInstances[i];

			if ( pcSprite->m_pcImage != NULL )
			{
				pcSprite->m_cPosition += cDelta;

				IRect cRect = pcSprite->m_cBounds + pcSprite->m_cPosition - pcSprite->m_cHotSpot;
				IPoint cClipOff  = cRect.LeftTop() - cLeftTop;

				pcSprite->Draw( pcTmp, cClipOff );
			}
		}

#if 0
		if ( cTotRect.IsValid() )
		{
			pcDriver->BltBitmap( m_pcTarget, pcTmp, cTotRect.OffsetToCopy(0,0), cLeftTop, B_OP_COPY );
		}
#endif
		for ( int i = s_cInstances.size() - 1 ; i >= 0 ; --i )
		{
			SrvSprite* pcSprite = s_cInstances[i];

			if ( pcSprite->m_pcImage == NULL )
			{
				pcSprite->m_cPosition += cDelta;
				pcSprite->Draw();
			}
		}
		delete pcTmp;
	}
	s_cLock.Unlock();
}

