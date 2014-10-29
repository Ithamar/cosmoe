/*
 *  The Cosmoe application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SupportDefs.h>
#include <isa_io.h>

#include <vesa_gfx.h>
#include <kernel.h>

#include <Bitmap.h>
#include "ServerBitmap.h"
#include "ServerCursor.h"

#include "vesadrv.h"
#include <IRect.h>





static area_id        g_nFrameBufArea = -1;


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

VesaDriver::VesaDriver() :
	m_nCurrentMode(0),
	m_nFrameBufferSize(0),
	m_nFrameBufferOffset(0)
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

VesaDriver::~VesaDriver()
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::InitModes( void )
{
	Vesa_Info_s      sVesaInfo;
	VESA_Mode_Info_s sModeInfo;
	uint16           anModes[1024];
	int              nModeCount;
	int              i=0;

	strcpy( sVesaInfo.VesaSignature, "VBE2" );

	nModeCount = get_vesa_info( &sVesaInfo, anModes, 1024 );

	if ( nModeCount <= 0 )
	{
		dbprintf( "Error: VesaDriver::InitModes() no VESA20 modes found\n" );
		return( false );
	}

//    dbprintf( "Found %d vesa modes\n", nModeCount );

	int nPagedCount  = 0;
	int nPlanarCount = 0;
	int nBadCount    = 0;

	for( i = 0 ; i < nModeCount ; ++i )
	{
		get_vesa_mode_info( &sModeInfo, anModes[i] );

		if( sModeInfo.PhysBasePtr == 0 ) // We must have a linear frame buffer
		{
			nPagedCount++;
			continue;
		}

		if( sModeInfo.BitsPerPixel < 8 )
		{
			nPlanarCount++;
			continue;
		}

		if ( sModeInfo.NumberOfPlanes != 1 )
		{
			nPlanarCount++;
			continue;
		}

		if ( sModeInfo.BitsPerPixel != 15 && sModeInfo.BitsPerPixel != 16 && sModeInfo.BitsPerPixel != 32 )
		{
			nBadCount++;
			continue;
		}

		if ( sModeInfo.RedMaskSize == 0 && sModeInfo.GreenMaskSize == 0 && sModeInfo.BlueMaskSize == 0 &&
			sModeInfo.RedFieldPosition == 0 && sModeInfo.GreenFieldPosition == 0 && sModeInfo.BlueFieldPosition == 0 )
		{
			m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
											B_COLOR_8_BIT, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
		}
		else if ( sModeInfo.RedMaskSize == 5 && sModeInfo.GreenMaskSize == 5 && sModeInfo.BlueMaskSize == 5 &&
					sModeInfo.RedFieldPosition == 10 && sModeInfo.GreenFieldPosition == 5 && sModeInfo.BlueFieldPosition == 0 )
		{
			m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
											B_RGB15, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
		}
		else if ( sModeInfo.RedMaskSize == 5 && sModeInfo.GreenMaskSize == 6 && sModeInfo.BlueMaskSize == 5 &&
					sModeInfo.RedFieldPosition == 11 && sModeInfo.GreenFieldPosition == 5 && sModeInfo.BlueFieldPosition == 0 )
		{
			m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
											B_RGB16, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
		}
		else if ( sModeInfo.BitsPerPixel == 32 && sModeInfo.RedMaskSize == 8 && sModeInfo.GreenMaskSize == 8 && sModeInfo.BlueMaskSize == 8 &&
					sModeInfo.RedFieldPosition == 16 && sModeInfo.GreenFieldPosition == 8 && sModeInfo.BlueFieldPosition == 0 )
		{
			m_cModeList.push_back( VesaMode( sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BytesPerScanLine,
											B_RGB32, anModes[i] | 0x4000, sModeInfo.PhysBasePtr ) );
		}
		else
		{
			dbprintf( "Found unsupported video mode: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d\n",
					sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine,
					sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize,
					sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition );
		}
#if 0
		dbprintf( "Mode %04x: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d (%p)\n", anModes[i],
				sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine,
				sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize,
				sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition, (void*)sModeInfo.PhysBasePtr );
#endif        
	}
	dbprintf( "Found total of %d VESA modes. Valid: %d, Paged: %d, Planar: %d, Bad: %d\n",
			nModeCount, m_cModeList.size(), nPagedCount, nPlanarCount, nBadCount );
	return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not

	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
area_id VesaDriver::Initialize(void)
{
	if ( InitModes() )
	{
		m_nFrameBufferSize = 1024 * 1024 * 4;
//      m_pFrameBuffer = NULL;
		g_nFrameBufArea = create_area( "framebuffer", NULL/*(void**) &m_pFrameBuffer*/, m_nFrameBufferSize,
									AREA_FULL_ACCESS, B_NO_LOCK );
		return( g_nFrameBufArea );
	}
	return( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

/*!
	\brief Shuts down the driver's video subsystem

	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void VesaDriver::Shutdown(void)
{
}

//----------------------------------------------------------------------------
// NAME
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int VesaDriver::GetScreenModeCount()
{
	return( m_cModeList.size() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int VesaDriver::SetScreenMode( int nWidth, int nHeight, color_space eColorSpc,
							int nPosH, int nPosV, int nSizeH, int nSizeV, float vRefreshRate )
{
	m_nCurrentMode  = -1;

	for ( int i = GetScreenModeCount() - 1 ; i >= 0 ; --i )
	{
		if ( m_cModeList[i].m_nWidth == nWidth && m_cModeList[i].m_nHeight == nHeight && m_cModeList[i].m_eColorSpace == eColorSpc ) {
			m_nCurrentMode = i;
			break;
		}
	}

	if ( m_nCurrentMode >= 0  )
	{
		m_nFrameBufferOffset = m_cModeList[m_nCurrentMode].m_nFrameBuffer & ~PAGE_MASK;
		if ( SetVesaMode( m_cModeList[m_nCurrentMode].m_nVesaMode ) )
		{
			return( 0 );
		}
	}
	return( -1 );
}

int VesaDriver::GetHorizontalRes()
{
	return( m_cModeList[m_nCurrentMode].m_nWidth );
}

int VesaDriver::GetVerticalRes()
{
	return( m_cModeList[m_nCurrentMode].m_nHeight );
}

int VesaDriver::GetBytesPerLine()
{
	return( m_cModeList[m_nCurrentMode].m_nBytesPerLine );
}

color_space VesaDriver::GetColorSpace()
{
	return( m_cModeList[m_nCurrentMode].m_eColorSpace );
}


void VesaDriver::SetColor( int nIndex, const rgb_color& sColor )
{
	outb_p( nIndex, 0x3c8 );
	outb_p( sColor.red >> 2, 0x3c9 );
	outb_p( sColor.green >> 2, 0x3c9 );
	outb_p( sColor.blue >> 2, 0x3c9 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::SetVesaMode( uint32 nMode )
{
	return ( set_vesa_mode( nMode ) == 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
/*
bool VesaDriver::StrokeLine(ServerBitmap* psBitMap, BRect clip,
							BPoint start, BPoint end,
							const rgb_color& color, int mode)
{
	DisplayDriver::StrokeLine( psBitMap, clip, start, end, color, mode );
	return true;
}
*/

/*!
	\brief Changes the cursor.
	\param cursor The new cursor. Guaranteed to be non-NULL.

	The driver does not take ownership of the given cursor. Subclasses should make
	a copy of the cursor passed to it. The default version of this function hides the
	cursor, replaces it, and shows the cursor if previously visible.
*/
void VesaDriver::SetCursor(ServerCursor *cursor)
{
	// Hide the sprite layer and delete the current cursor sprite
	SrvSprite::Hide();
	delete m_pcMouseSprite;

	// Use the default method to set the cursor bitmap
	DisplayDriver::SetCursor(cursor);

	// Create a new sprite based on the cursor bitmap
	m_pcMouseSprite = new SrvSprite( IRect(_GetCursor()->Bounds()),
										 m_cMousePos,
										 IPoint(_GetCursor()->GetHotSpot()),
										 GetTarget(),
										 _GetCursor() );

	// Show the sprite layer, revealing the new cursor
	SrvSprite::Unhide();
}

/*!
	\brief Moves the cursor to the given point.

	The coordinates passed to MoveCursorTo are guaranteed to be within the frame buffer's
	range, but the cursor data itself will need to be clipped. A check to see if the
	cursor is obscured should be made and if so, a call to _SetCursorObscured(false)
	should be made the cursor in addition to displaying at the passed coordinates.
*/
void VesaDriver::MoveCursorTo(float x, float y)
{
	IPoint cNewPos(x, y);

	if ( m_pcMouseSprite != NULL )
	{
		m_pcMouseSprite->MoveTo( cNewPos );
	}
	m_cMousePos = cNewPos;
}


/*!
	\brief Hides the cursor.

	Hide calls are not nestable, unlike that of the BApplication class. Subclasses should
	call _SetCursorHidden(true) somewhere within this function to ensure that data is
	maintained accurately. Subclasses must include a call to DisplayDriver::HideCursor
	for proper state tracking.
*/
void VesaDriver::HideCursor(void)
{
	DisplayDriver::HideCursor();

	if ( m_pcMouseSprite == NULL )
	{
		dbprintf( "Warning: DisplayDriver::HideCursor() called while mouse hidden\n" );
	}
	delete m_pcMouseSprite;
	m_pcMouseSprite = NULL;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool VesaDriver::BltBitmap( ServerBitmap* dstbm, ServerBitmap* srcbm, IRect cSrcRect, IPoint cDstPos, int nMode )
{
	return( DisplayDriver::BltBitmap( dstbm, srcbm, cSrcRect, cDstPos, nMode ) );
}
