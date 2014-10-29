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

#include <Bitmap.h>
#include "ServerBitmap.h"
#include "ServerCursor.h"

#include "vesadrv.h"
#include "ServerFont.h"

#define DEBUG_VESA_DRIVER

#ifdef DEBUG_VESA_DRIVER
#include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

#define PAGE_MASK	(~(B_PAGE_SIZE-1))

#define RAS_OFFSET8( ptr, x, y, bpl )  (((uint8*)(ptr)) + (x) + (y) * (bpl))
#define RAS_OFFSET16( ptr, x, y, bpl ) ((uint16*)(((uint8*)(ptr)) + (x*2) + (y) * (bpl)))
#define RAS_OFFSET32( ptr, x, y, bpl ) ((uint32*)(((uint8*)(ptr)) + (x*4) + (y) * (bpl)))

rgb_color g_asDefaultPallette[] = {
    { 0x00, 0x00, 0x00, 0x00 },        // 0
    { 0x08, 0x08, 0x08, 0x00 },
    { 0x10, 0x10, 0x10, 0x00 },
    { 0x18, 0x18, 0x18, 0x00 },
    { 0x20, 0x20, 0x20, 0x00 },
    { 0x28, 0x28, 0x28, 0x00 },        // 5
    { 0x30, 0x30, 0x30, 0x00 },
    { 0x38, 0x38, 0x38, 0x00 },
    { 0x40, 0x40, 0x40, 0x00 },
    { 0x48, 0x48, 0x48, 0x00 },
    { 0x50, 0x50, 0x50, 0x00 },        // 10
    { 0x58, 0x58, 0x58, 0x00 },
    { 0x60, 0x60, 0x60, 0x00 },
    { 0x68, 0x68, 0x68, 0x00 },
    { 0x70, 0x70, 0x70, 0x00 },
    { 0x78, 0x78, 0x78, 0x00 },        // 15
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x88, 0x88, 0x88, 0x00 },
    { 0x90, 0x90, 0x90, 0x00 },
    { 0x98, 0x98, 0x98, 0x00 },
    { 0xa0, 0xa0, 0xa0, 0x00 },        // 20
    { 0xa8, 0xa8, 0xa8, 0x00 },
    { 0xb0, 0xb0, 0xb0, 0x00 },
    { 0xb8, 0xb8, 0xb8, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc8, 0xc8, 0xc8, 0x00 },        // 25
    { 0xd0, 0xd0, 0xd0, 0x00 },
    { 0xd9, 0xd9, 0xd9, 0x00 },
    { 0xe2, 0xe2, 0xe2, 0x00 },
    { 0xeb, 0xeb, 0xeb, 0x00 },
    { 0xf5, 0xf5, 0xf5, 0x00 },        // 30
    { 0xfe, 0xfe, 0xfe, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0x00, 0xe5, 0x00 },
    { 0x00, 0x00, 0xcc, 0x00 },
    { 0x00, 0x00, 0xb3, 0x00 },        // 35
    { 0x00, 0x00, 0x9a, 0x00 },
    { 0x00, 0x00, 0x81, 0x00 },
    { 0x00, 0x00, 0x69, 0x00 },
    { 0x00, 0x00, 0x50, 0x00 },
    { 0x00, 0x00, 0x37, 0x00 },        // 40
    { 0x00, 0x00, 0x1e, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xe4, 0x00, 0x00, 0x00 },
    { 0xcb, 0x00, 0x00, 0x00 },
    { 0xb2, 0x00, 0x00, 0x00 },        // 45
    { 0x99, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x69, 0x00, 0x00, 0x00 },
    { 0x50, 0x00, 0x00, 0x00 },
    { 0x37, 0x00, 0x00, 0x00 },        // 50
    { 0x1e, 0x00, 0x00, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xe4, 0x00, 0x00 },
    { 0x00, 0xcb, 0x00, 0x00 },
    { 0x00, 0xb2, 0x00, 0x00 },        // 55
    { 0x00, 0x99, 0x00, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x69, 0x00, 0x00 },
    { 0x00, 0x50, 0x00, 0x00 },
    { 0x00, 0x37, 0x00, 0x00 },        // 60
    { 0x00, 0x1e, 0x00, 0x00 },
    { 0x00, 0x98, 0x33, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 },
    { 0xcb, 0xff, 0xff, 0x00 },
    { 0xcb, 0xff, 0xcb, 0x00 },        // 65
    { 0xcb, 0xff, 0x98, 0x00 },
    { 0xcb, 0xff, 0x66, 0x00 },
    { 0xcb, 0xff, 0x33, 0x00 },
    { 0xcb, 0xff, 0x00, 0x00 },
    { 0x98, 0xff, 0xff, 0x00 },
    { 0x98, 0xff, 0xcb, 0x00 },
    { 0x98, 0xff, 0x98, 0x00 },
    { 0x98, 0xff, 0x66, 0x00 },
    { 0x98, 0xff, 0x33, 0x00 },
    { 0x98, 0xff, 0x00, 0x00 },
    { 0x66, 0xff, 0xff, 0x00 },
    { 0x66, 0xff, 0xcb, 0x00 },
    { 0x66, 0xff, 0x98, 0x00 },
    { 0x66, 0xff, 0x66, 0x00 },
    { 0x66, 0xff, 0x33, 0x00 },
    { 0x66, 0xff, 0x00, 0x00 },
    { 0x33, 0xff, 0xff, 0x00 },
    { 0x33, 0xff, 0xcb, 0x00 },
    { 0x33, 0xff, 0x98, 0x00 },
    { 0x33, 0xff, 0x66, 0x00 },
    { 0x33, 0xff, 0x33, 0x00 },
    { 0x33, 0xff, 0x00, 0x00 },
    { 0xff, 0x98, 0xff, 0x00 },
    { 0xff, 0x98, 0xcb, 0x00 },
    { 0xff, 0x98, 0x98, 0x00 },
    { 0xff, 0x98, 0x66, 0x00 },
    { 0xff, 0x98, 0x33, 0x00 },
    { 0xff, 0x98, 0x00, 0x00 },
    { 0x00, 0x66, 0xff, 0x00 },
    { 0x00, 0x66, 0xcb, 0x00 },
    { 0xcb, 0xcb, 0xff, 0x00 },
    { 0xcb, 0xcb, 0xcb, 0x00 },
    { 0xcb, 0xcb, 0x98, 0x00 },
    { 0xcb, 0xcb, 0x66, 0x00 },
    { 0xcb, 0xcb, 0x33, 0x00 },
    { 0xcb, 0xcb, 0x00, 0x00 },
    { 0x98, 0xcb, 0xff, 0x00 },
    { 0x98, 0xcb, 0xcb, 0x00 },
    { 0x98, 0xcb, 0x98, 0x00 },
    { 0x98, 0xcb, 0x66, 0x00 },
    { 0x98, 0xcb, 0x33, 0x00 },
    { 0x98, 0xcb, 0x00, 0x00 },
    { 0x66, 0xcb, 0xff, 0x00 },
    { 0x66, 0xcb, 0xcb, 0x00 },
    { 0x66, 0xcb, 0x98, 0x00 },
    { 0x66, 0xcb, 0x66, 0x00 },
    { 0x66, 0xcb, 0x33, 0x00 },
    { 0x66, 0xcb, 0x00, 0x00 },
    { 0x33, 0xcb, 0xff, 0x00 },
    { 0x33, 0xcb, 0xcb, 0x00 },
    { 0x33, 0xcb, 0x98, 0x00 },
    { 0x33, 0xcb, 0x66, 0x00 },
    { 0x33, 0xcb, 0x33, 0x00 },
    { 0x33, 0xcb, 0x00, 0x00 },
    { 0xff, 0x66, 0xff, 0x00 },
    { 0xff, 0x66, 0xcb, 0x00 },
    { 0xff, 0x66, 0x98, 0x00 },
    { 0xff, 0x66, 0x66, 0x00 },
    { 0xff, 0x66, 0x33, 0x00 },
    { 0xff, 0x66, 0x00, 0x00 },
    { 0x00, 0x66, 0x98, 0x00 },
    { 0x00, 0x66, 0x66, 0x00 },
    { 0xcb, 0x98, 0xff, 0x00 },
    { 0xcb, 0x98, 0xcb, 0x00 },
    { 0xcb, 0x98, 0x98, 0x00 },
    { 0xcb, 0x98, 0x66, 0x00 },
    { 0xcb, 0x98, 0x33, 0x00 },
    { 0xcb, 0x98, 0x00, 0x00 },
    { 0x98, 0x98, 0xff, 0x00 },
    { 0x98, 0x98, 0xcb, 0x00 },
    { 0x98, 0x98, 0x98, 0x00 },
    { 0x98, 0x98, 0x66, 0x00 },
    { 0x98, 0x98, 0x33, 0x00 },
    { 0x98, 0x98, 0x00, 0x00 },
    { 0x66, 0x98, 0xff, 0x00 },
    { 0x66, 0x98, 0xcb, 0x00 },
    { 0x66, 0x98, 0x98, 0x00 },
    { 0x66, 0x98, 0x66, 0x00 },
    { 0x66, 0x98, 0x33, 0x00 },
    { 0x66, 0x98, 0x00, 0x00 },
    { 0x33, 0x98, 0xff, 0x00 },
    { 0x33, 0x98, 0xcb, 0x00 },
    { 0x33, 0x98, 0x98, 0x00 },
    { 0x33, 0x98, 0x66, 0x00 },
    { 0x33, 0x98, 0x33, 0x00 },
    { 0x33, 0x98, 0x00, 0x00 },
    { 0xe6, 0x86, 0x00, 0x00 },
    { 0xff, 0x33, 0xcb, 0x00 },
    { 0xff, 0x33, 0x98, 0x00 },
    { 0xff, 0x33, 0x66, 0x00 },
    { 0xff, 0x33, 0x33, 0x00 },
    { 0xff, 0x33, 0x00, 0x00 },
    { 0x00, 0x66, 0x33, 0x00 },
    { 0x00, 0x66, 0x00, 0x00 },
    { 0xcb, 0x66, 0xff, 0x00 },
    { 0xcb, 0x66, 0xcb, 0x00 },
    { 0xcb, 0x66, 0x98, 0x00 },
    { 0xcb, 0x66, 0x66, 0x00 },
    { 0xcb, 0x66, 0x33, 0x00 },
    { 0xcb, 0x66, 0x00, 0x00 },
    { 0x98, 0x66, 0xff, 0x00 },
    { 0x98, 0x66, 0xcb, 0x00 },
    { 0x98, 0x66, 0x98, 0x00 },
    { 0x98, 0x66, 0x66, 0x00 },
    { 0x98, 0x66, 0x33, 0x00 },
    { 0x98, 0x66, 0x00, 0x00 },
    { 0x66, 0x66, 0xff, 0x00 },
    { 0x66, 0x66, 0xcb, 0x00 },
    { 0x66, 0x66, 0x98, 0x00 },
    { 0x66, 0x66, 0x66, 0x00 },
    { 0x66, 0x66, 0x33, 0x00 },
    { 0x66, 0x66, 0x00, 0x00 },
    { 0x33, 0x66, 0xff, 0x00 },
    { 0x33, 0x66, 0xcb, 0x00 },
    { 0x33, 0x66, 0x98, 0x00 },
    { 0x33, 0x66, 0x66, 0x00 },
    { 0x33, 0x66, 0x33, 0x00 },
    { 0x33, 0x66, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0x00, 0xcb, 0x00 },
    { 0xff, 0x00, 0x98, 0x00 },
    { 0xff, 0x00, 0x66, 0x00 },
    { 0xff, 0x00, 0x33, 0x00 },
    { 0xff, 0xaf, 0x13, 0x00 },
    { 0x00, 0x33, 0xff, 0x00 },
    { 0x00, 0x33, 0xcb, 0x00 },
    { 0xcb, 0x33, 0xff, 0x00 },
    { 0xcb, 0x33, 0xcb, 0x00 },
    { 0xcb, 0x33, 0x98, 0x00 },
    { 0xcb, 0x33, 0x66, 0x00 },
    { 0xcb, 0x33, 0x33, 0x00 },
    { 0xcb, 0x33, 0x00, 0x00 },
    { 0x98, 0x33, 0xff, 0x00 },
    { 0x98, 0x33, 0xcb, 0x00 },
    { 0x98, 0x33, 0x98, 0x00 },
    { 0x98, 0x33, 0x66, 0x00 },
    { 0x98, 0x33, 0x33, 0x00 },
    { 0x98, 0x33, 0x00, 0x00 },
    { 0x66, 0x33, 0xff, 0x00 },
    { 0x66, 0x33, 0xcb, 0x00 },
    { 0x66, 0x33, 0x98, 0x00 },
    { 0x66, 0x33, 0x66, 0x00 },
    { 0x66, 0x33, 0x33, 0x00 },
    { 0x66, 0x33, 0x00, 0x00 },
    { 0x33, 0x33, 0xff, 0x00 },
    { 0x33, 0x33, 0xcb, 0x00 },
    { 0x33, 0x33, 0x98, 0x00 },
    { 0x33, 0x33, 0x66, 0x00 },
    { 0x33, 0x33, 0x33, 0x00 },
    { 0x33, 0x33, 0x00, 0x00 },
    { 0xff, 0xcb, 0x66, 0x00 },
    { 0xff, 0xcb, 0x98, 0x00 },
    { 0xff, 0xcb, 0xcb, 0x00 },
    { 0xff, 0xcb, 0xff, 0x00 },
    { 0x00, 0x33, 0x98, 0x00 },
    { 0x00, 0x33, 0x66, 0x00 },
    { 0x00, 0x33, 0x33, 0x00 },
    { 0x00, 0x33, 0x00, 0x00 },
    { 0xcb, 0x00, 0xff, 0x00 },
    { 0xcb, 0x00, 0xcb, 0x00 },
    { 0xcb, 0x00, 0x98, 0x00 },
    { 0xcb, 0x00, 0x66, 0x00 },
    { 0xcb, 0x00, 0x33, 0x00 },
    { 0xff, 0xe3, 0x46, 0x00 },
    { 0x98, 0x00, 0xff, 0x00 },
    { 0x98, 0x00, 0xcb, 0x00 },
    { 0x98, 0x00, 0x98, 0x00 },
    { 0x98, 0x00, 0x66, 0x00 },
    { 0x98, 0x00, 0x33, 0x00 },
    { 0x98, 0x00, 0x00, 0x00 },
    { 0x66, 0x00, 0xff, 0x00 },
    { 0x66, 0x00, 0xcb, 0x00 },
    { 0x66, 0x00, 0x98, 0x00 },
    { 0x66, 0x00, 0x66, 0x00 },
    { 0x66, 0x00, 0x33, 0x00 },
    { 0x66, 0x00, 0x00, 0x00 },
    { 0x33, 0x00, 0xff, 0x00 },
    { 0x33, 0x00, 0xcb, 0x00 },
    { 0x33, 0x00, 0x98, 0x00 },
    { 0x33, 0x00, 0x66, 0x00 },
    { 0x33, 0x00, 0x33, 0x00 },
    { 0x33, 0x00, 0x00, 0x00 },
    { 0xff, 0xcb, 0x33, 0x00 },
    { 0xff, 0xcb, 0x00, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0x33, 0x00 },
    { 0xff, 0xff, 0x66, 0x00 },
    { 0xff, 0xff, 0x98, 0x00 },
    { 0xff, 0xff, 0xcb, 0x00 },
    { 0xff, 0xff, 0xff, 0xff }
};


static inline void blit_convert_alpha( ServerBitmap* pcDst, ServerBitmap* pcSrc,
                                       const IRect& cSrcRect, const IPoint& cDstPos );
static inline void blit_convert_over( ServerBitmap* pcDst, ServerBitmap* pcSrc,
                                      const IRect& cSrcRect, const IPoint& cDstPos );
static inline void blit_convert_copy( ServerBitmap* pcDst, ServerBitmap* pcSrc,
                                      const IRect& cSrcRect, const IPoint& cDstPos );

static inline void BitBlit( ServerBitmap *sbm, ServerBitmap *dbm,
                            int sx, int sy, int dx, int dy, int w, int h );

static void invert_line15( ServerBitmap* pcBitmap, const IRect& cClip,
                           int x1, int y1, int x2, int y2 );
static void invert_line16( ServerBitmap* pcBitmap, const IRect& cClip,
                           int x1, int y1, int x2, int y2 );
static void invert_line32( ServerBitmap* pcBitmap, const IRect& cClip,
                           int x1, int y1, int x2, int y2 );
static void draw_line16( ServerBitmap* pcBitmap, const IRect& cClip,
                         int x1, int y1, int x2, int y2, uint16 nColor );
static void draw_line32( ServerBitmap* pcBitmap, const IRect& cClip,
                         int x1, int y1, int x2, int y2, uint32 nColor );

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
	// Hack alert: we hardcode the screen dimensions here
	_SetWidth(800);
	_SetHeight(600);
	_SetDepth(32);
	_SetBytesPerRow(800);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

VesaDriver::~VesaDriver(void)
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
		STRACE(( "Error: VesaDriver::InitModes() no VESA20 modes found\n" ));
		return( false );
	}

//    STRACE(( "Found %d vesa modes\n", nModeCount ));

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
			STRACE(( "Found unsupported video mode: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d\n",
					sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine,
					sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize,
					sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition ));
		}
#if 0
		STRACE(( "Mode %04x: %dx%d %d BPP %d BPL - %d:%d:%d, %d:%d:%d (%p)\n", anModes[i],
				sModeInfo.XResolution, sModeInfo.YResolution, sModeInfo.BitsPerPixel, sModeInfo.BytesPerScanLine,
				sModeInfo.RedMaskSize, sModeInfo.GreenMaskSize, sModeInfo.BlueMaskSize,
				sModeInfo.RedFieldPosition, sModeInfo.GreenFieldPosition, sModeInfo.BlueFieldPosition, (void*)sModeInfo.PhysBasePtr ));
#endif        
	}
	STRACE(( "Found total of %d VESA modes. Valid: %d, Paged: %d, Planar: %d, Bad: %d\n",
			nModeCount, m_cModeList.size(), nPagedCount, nPlanarCount, nBadCount ));
	return( true );
}


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
		g_nFrameBufArea = create_area("framebuffer", NULL, B_ANY_ADDRESS, m_nFrameBufferSize,
									B_NO_LOCK, (B_READ_AREA | B_WRITE_AREA));
		return( g_nFrameBufArea );
	}
	return( -1 );
}


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

bool VesaDriver::GetScreenModeDesc( int nIndex, ScreenMode* psMode )
{
	if ( nIndex >= 0 && nIndex < int(m_cModeList.size()) )
	{
		*psMode = m_cModeList[nIndex];
		return( true );
	}

	return( false );
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
		if ( m_cModeList[i].mWidth == nWidth && m_cModeList[i].mHeight == nHeight && m_cModeList[i].mColorSpace == eColorSpc )
		{
			m_nCurrentMode = i;
			break;
		}
	}

	if ( m_nCurrentMode >= 0  )
	{
		m_nFrameBufferOffset = m_cModeList[m_nCurrentMode].m_nFrameBuffer & ~PAGE_MASK;
		if ( SetVesaMode( m_cModeList[m_nCurrentMode].m_nVesaMode ) )
		{
			_SetWidth(nWidth);
			_SetHeight(nHeight);
			_SetBytesPerRow(nWidth);
			return( 0 );
		}
	}
	return( -1 );
}

int VesaDriver::GetHorizontalRes()
{
	return( m_cModeList[m_nCurrentMode].mWidth );
}

int VesaDriver::GetVerticalRes()
{
	return( m_cModeList[m_nCurrentMode].mHeight );
}

int VesaDriver::GetBytesPerLine()
{
	return( m_cModeList[m_nCurrentMode].mBytesPerLine );
}

color_space VesaDriver::GetColorSpace()
{
	return( m_cModeList[m_nCurrentMode].mColorSpace );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

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

bool VesaDriver::BltBitmap( ServerBitmap *dstbm, ServerBitmap *srcbm,
								IRect cSrcRect, IPoint cDstPos, int nMode )
{
	switch( nMode )
	{
		case B_OP_COPY:
			if ( srcbm->ColorSpace() == dstbm->ColorSpace() ) {
				int sx = cSrcRect.left;
				int sy = cSrcRect.top;
				int dx = cDstPos.x;
				int dy = cDstPos.y;
				int w  = cSrcRect.Width()+1;
				int h  = cSrcRect.Height()+1;

				BitBlit( srcbm, dstbm, sx, sy, dx, dy, w, h );
			} else {
				blit_convert_copy( dstbm, srcbm, cSrcRect, cDstPos );
			}
			break;
		case B_OP_OVER:
			blit_convert_over( dstbm, srcbm, cSrcRect, cDstPos );
			break;
		case B_OP_BLEND:
			if ( srcbm->ColorSpace() == B_RGB32 ) {
				blit_convert_alpha( dstbm, srcbm, cSrcRect, cDstPos );
			} else {
				blit_convert_over( dstbm, srcbm, cSrcRect, cDstPos );
			}
			break;
	}
	return( true );
}


void VesaDriver::OldStrokeLine(ServerBitmap* psBitmap, BRect clip,
							BPoint start, BPoint end,
							const rgb_color& color, int mode)
{
	switch( mode )
	{
		case B_OP_COPY:
		case B_OP_OVER:
		default:
			switch( psBitmap->ColorSpace() )
			{
				case B_RGB15:
					draw_line16( psBitmap, clip, start.x, start.y, end.x, end.y, COL_TO_RGB15( color ) );
					break;
				case B_RGB16:
					draw_line16( psBitmap, clip, start.x, start.y, end.x, end.y, COL_TO_RGB16( color ) );
					break;
				case B_RGB32:
					draw_line32( psBitmap, clip, start.x, start.y, end.x, end.y, COL_TO_RGB32( color ) );
					break;
				default:
					STRACE(( "VesaDriver::StrokeLine() unknown color space %d\n", psBitmap->ColorSpace() ));
			}
			break;

		case B_OP_INVERT:
			switch( psBitmap->ColorSpace() )
			{
				case B_RGB15:
					invert_line15( psBitmap, clip, start.x, start.y, end.x, end.y );
					break;
				case B_RGB16:
					invert_line16( psBitmap, clip, start.x, start.y, end.x, end.y );
					break;
				case B_RGB32:
					invert_line32( psBitmap, clip, start.x, start.y, end.x, end.y );
					break;
				default:
					STRACE(( "VesaDriver::StrokeLine() unknown color space %d can't invert\n", psBitmap->ColorSpace() ));
			}
			break;
	}
}

bool VesaDriver::OldFillRect( ServerBitmap* pcBitmap, BRect r, const rgb_color& sColor )
{
	int BltX, BltY, BltW, BltH;

	BltX = r.left;
	BltY = r.top;
	BltW = r.IntegerWidth() + 1;
	BltH = r.IntegerHeight() + 1;

	switch( pcBitmap->ColorSpace() )
	{
		case B_RGB15:
			FillBlit16( (uint16*) &pcBitmap->Bits()[ BltY * pcBitmap->BytesPerRow() + BltX * 2 ],
						pcBitmap->BytesPerRow() / 2 - BltW, BltW, BltH, COL_TO_RGB15( sColor ) );
			break;
		case B_RGB16:
			FillBlit16( (uint16*) &pcBitmap->Bits()[ BltY * pcBitmap->BytesPerRow() + BltX * 2 ],
						pcBitmap->BytesPerRow() / 2 - BltW, BltW, BltH, COL_TO_RGB16( sColor ) );
			break;
		case B_RGB32:
			FillBlit32( (uint32*) &pcBitmap->Bits()[ BltY * pcBitmap->BytesPerRow() + BltX * 4 ],
						pcBitmap->BytesPerRow() / 4 - BltW, BltW, BltH, COL_TO_RGB32( sColor ) );
			break;
		default:
			STRACE(( "VesaDriver::FillRect() unknown color space %d\n", pcBitmap->ColorSpace() ));
	}

	return( true );
}


inline int32 BitsPerPixel( color_space eColorSpc )
{
    switch( eColorSpc )
    {
	case B_RGB32:
	case B_RGBA32:
	    return( 32 );
	case B_RGB24:
	    return( 24 );
	case B_RGB16:
	case B_RGB15:
	case B_RGBA15:
	    return( 16 );
	case B_COLOR_8_BIT:
	case B_GRAY8:
	    return( 8 );
	case B_GRAY1:
	    return( 1 );
	default:
	    printf( "BitsPerPixel() invalid color space %d\n", eColorSpc );
	    return( 8 );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static void draw_line16( ServerBitmap* pcBitmap, const IRect& cClip,
                         int x1, int y1, int x2, int y2, uint16 nColor )
{
	int nODeltaX = abs( x2 - x1 );
	int nODeltaY = abs( y2 - y1 );
	int ox1 = x1;
	int oy1 = y1;
	uint16* pRaster;
	int          nModulo = pcBitmap->BytesPerRow();

	if ( VesaDriver::ClipLine( cClip, &x1, &y1, &x2, &y2 ) == false ) {
		return;
	}
	int nDeltaX = abs( x2 - x1 );
	int nDeltaY = abs( y2 - y1 );

	if ( nODeltaX > nODeltaY )
	{
		int dinc1 = nODeltaY << 1;
		int dinc2 = (nODeltaY - nODeltaX) << 1;
		int d = dinc1 - nODeltaX;

		int nYStep;

		if ( ox1 != x1 || oy1 != y1 ) {
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaY*dinc2) + ((nClipDeltaX-nClipDeltaY)*dinc1));
		}

		if ( y1 > y2 ) {
			nYStep = -nModulo;
		} else {
			nYStep = nModulo;
		}
		if ( x1 > x2 ) {
			nYStep = -nYStep;
			pRaster = (uint16*)(pcBitmap->Bits() + x2 * 2 + nModulo * y2);
		} else {
			pRaster = (uint16*)(pcBitmap->Bits() + x1 * 2 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaX ; ++i )
		{
			*pRaster = nColor;
			if ( d < 0 ) {
				d += dinc1;
			} else {
				d += dinc2;
				pRaster = (uint16*)(((uint8*)pRaster) + nYStep);
			}
			pRaster++;
		}
	}
	else
	{
		int dinc1 = nODeltaX << 1;
		int d = dinc1 - nODeltaY;
		int dinc2 = (nODeltaX - nODeltaY) << 1;
		int nXStep;

		if ( ox1 != x1 || oy1 != y1 ) {
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaX*dinc2) + ((nClipDeltaY-nClipDeltaX)*dinc1));
		}


		if ( x1 > x2 ) {
			nXStep = -sizeof(uint16);
		} else {
			nXStep = sizeof(uint16);
		}
		if ( y1 > y2 ) {
			nXStep = -nXStep;
			pRaster = (uint16*)(pcBitmap->Bits() + x2 * 2 + nModulo * y2);
		} else {
			pRaster = (uint16*)(pcBitmap->Bits() + x1 * 2 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaY ; ++i )
		{
			*pRaster = nColor;
			if ( d < 0 ) {
				d += dinc1;
			} else {
				d += dinc2;
				pRaster = (uint16*)(((uint8*)pRaster) + nXStep);
			}
			pRaster = (uint16*)(((uint8*)pRaster) + nModulo);
		}
	}
}

static void invert_line16( ServerBitmap* pcBitmap, const IRect& cClip,
                           int x1, int y1, int x2, int y2 )
{
	int nODeltaX = abs( x2 - x1 );
	int nODeltaY = abs( y2 - y1 );
	int ox1 = x1;
	int oy1 = y1;
	uint16* pRaster;
	int          nModulo = pcBitmap->BytesPerRow();

	if ( VesaDriver::ClipLine( cClip, &x1, &y1, &x2, &y2 ) == false ) {
		return;
	}
	int nDeltaX = abs( x2 - x1 );
	int nDeltaY = abs( y2 - y1 );

	if ( nODeltaX > nODeltaY )
	{
		int dinc1 = nODeltaY << 1;
		int dinc2 = (nODeltaY - nODeltaX) << 1;
		int d = dinc1 - nODeltaX;

		int nYStep;

		if ( ox1 != x1 || oy1 != y1 ) {
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaY*dinc2) + ((nClipDeltaX-nClipDeltaY)*dinc1));
		}

		if ( y1 > y2 ) {
			nYStep = -nModulo;
		} else {
			nYStep = nModulo;
		}
		if ( x1 > x2 ) {
			nYStep = -nYStep;
			pRaster = (uint16*)(pcBitmap->Bits() + x2 * 2 + nModulo * y2);
		} else {
			pRaster = (uint16*)(pcBitmap->Bits() + x1 * 2 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaX ; ++i )
		{
			rgb_color sColor = RGB16_TO_COL( *pRaster );
			sColor.red   = 255 - sColor.red;
			sColor.green = 255 - sColor.green;
			sColor.blue  = 255 - sColor.blue;
			*pRaster = COL_TO_RGB16( sColor );

			if ( d < 0 ) {
				d += dinc1;
			} else {
				d += dinc2;
				pRaster = (uint16*)(((uint8*)pRaster) + nYStep);
			}
			pRaster++;
		}
	}
	else
	{
		int dinc1 = nODeltaX << 1;
		int d = dinc1 - nODeltaY;
		int dinc2 = (nODeltaX - nODeltaY) << 1;
		int nXStep;

		if ( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaX*dinc2) + ((nClipDeltaY-nClipDeltaX)*dinc1));
		}

		if ( x1 > x2 )
		{
			nXStep = -sizeof(uint16);
		}
		else
		{
			nXStep = sizeof(uint16);
		}

		if ( y1 > y2 )
		{
			nXStep = -nXStep;
			pRaster = (uint16*)(pcBitmap->Bits() + x2 * 2 + nModulo * y2);
		}
		else
		{
			pRaster = (uint16*)(pcBitmap->Bits() + x1 * 2 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaY ; ++i )
		{
			rgb_color sColor = RGB16_TO_COL( *pRaster );
			sColor.red   = 255 - sColor.red;
			sColor.green = 255 - sColor.green;
			sColor.blue  = 255 - sColor.blue;
			*pRaster = COL_TO_RGB16( sColor );

			if ( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = (uint16*)(((uint8*)pRaster) + nXStep);
			}
			pRaster = (uint16*)(((uint8*)pRaster) + nModulo);
		}
	}
}

static void invert_line15( ServerBitmap* pcBitmap, const IRect& cClip,
                           int x1, int y1, int x2, int y2 )
{
	int nODeltaX = abs( x2 - x1 );
	int nODeltaY = abs( y2 - y1 );
	int ox1 = x1;
	int oy1 = y1;
	uint16* pRaster;
	int          nModulo = pcBitmap->BytesPerRow();

	if ( VesaDriver::ClipLine( cClip, &x1, &y1, &x2, &y2 ) == false )
		return;

	int nDeltaX = abs( x2 - x1 );
	int nDeltaY = abs( y2 - y1 );

	if ( nODeltaX > nODeltaY )
	{
		int dinc1 = nODeltaY << 1;
		int dinc2 = (nODeltaY - nODeltaX) << 1;
		int d = dinc1 - nODeltaX;

		int nYStep;

		if ( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaY*dinc2) + ((nClipDeltaX-nClipDeltaY)*dinc1));
		}

		if ( y1 > y2 )
		{
			nYStep = -nModulo;
		}
		else
		{
			nYStep = nModulo;
		}

		if ( x1 > x2 )
		{
			nYStep = -nYStep;
			pRaster = (uint16*)(pcBitmap->Bits() + x2 * 2 + nModulo * y2);
		}
		else
		{
			pRaster = (uint16*)(pcBitmap->Bits() + x1 * 2 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaX ; ++i )
		{
			rgb_color sColor = RGB15_TO_COL( *pRaster );
			sColor.red   = 255 - sColor.red;
			sColor.green = 255 - sColor.green;
			sColor.blue  = 255 - sColor.blue;
			*pRaster = COL_TO_RGB15( sColor );

			if ( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = (uint16*)(((uint8*)pRaster) + nYStep);
			}
			pRaster++;
		}
	}
	else
	{
		int dinc1 = nODeltaX << 1;
		int d = dinc1 - nODeltaY;
		int dinc2 = (nODeltaX - nODeltaY) << 1;
		int nXStep;

		if ( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaX*dinc2) + ((nClipDeltaY-nClipDeltaX)*dinc1));
		}


		if ( x1 > x2 ) {
			nXStep = -sizeof(uint16);
		} else {
			nXStep = sizeof(uint16);
		}
		if ( y1 > y2 ) {
			nXStep = -nXStep;
			pRaster = (uint16*)(pcBitmap->Bits() + x2 * 2 + nModulo * y2);
		} else {
			pRaster = (uint16*)(pcBitmap->Bits() + x1 * 2 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaY ; ++i )
		{
			rgb_color sColor = RGB15_TO_COL( *pRaster );
			sColor.red   = 255 - sColor.red;
			sColor.green = 255 - sColor.green;
			sColor.blue  = 255 - sColor.blue;
			*pRaster = COL_TO_RGB15( sColor );

			if ( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = (uint16*)(((uint8*)pRaster) + nXStep);
			}
			pRaster = (uint16*)(((uint8*)pRaster) + nModulo);
		}
	}
}

static void draw_line32( ServerBitmap* pcBitmap, const IRect& cClip,
                         int x1, int y1, int x2, int y2, uint32 nColor )
{
	int nODeltaX = abs( x2 - x1 );
	int nODeltaY = abs( y2 - y1 );
	int ox1 = x1;
	int oy1 = y1;
	uint32* pRaster;
	int     nModulo = pcBitmap->BytesPerRow();

	if ( VesaDriver::ClipLine( cClip, &x1, &y1, &x2, &y2 ) == false )
		return;

	int nDeltaX = abs( x2 - x1 );
	int nDeltaY = abs( y2 - y1 );

	if ( nODeltaX > nODeltaY )
	{
		int dinc1 = nODeltaY << 1;
		int dinc2 = (nODeltaY - nODeltaX) << 1;
		int d = dinc1 - nODeltaX;

		int nYStep;

		if ( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaY*dinc2) + ((nClipDeltaX-nClipDeltaY)*dinc1));
		}

		if ( y1 > y2 )
		{
			nYStep = -nModulo;
		}
		else
		{
			nYStep = nModulo;
		}
		if ( x1 > x2 )
		{
			nYStep = -nYStep;
			pRaster = (uint32*)(pcBitmap->Bits() + x2 * 4 + nModulo * y2);
		}
		else
		{
			pRaster = (uint32*)(pcBitmap->Bits() + x1 * 4 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaX ; ++i )
		{
			*pRaster = nColor;
			if ( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = (uint32*)(((uint8*)pRaster) + nYStep);
			}
			pRaster++;
		}
	}
	else
	{
		int dinc1 = nODeltaX << 1;
		int d = dinc1 - nODeltaY;
		int dinc2 = (nODeltaX - nODeltaY) << 1;
		int nXStep;

		if ( ox1 != x1 || oy1 != y1 ) {
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaX*dinc2) + ((nClipDeltaY-nClipDeltaX)*dinc1));
		}


		if ( x1 > x2 ) {
			nXStep = -sizeof(uint32);
		} else {
			nXStep = sizeof(uint32);
		}
		if ( y1 > y2 ) {
			nXStep = -nXStep;
			pRaster = (uint32*)(pcBitmap->Bits() + x2 * 4 + nModulo * y2);
		} else {
			pRaster = (uint32*)(pcBitmap->Bits() + x1 * 4 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaY ; ++i )
		{
			*pRaster = nColor;
			if ( d < 0 ) {
				d += dinc1;
			} else {
				d += dinc2;
				pRaster = (uint32*)(((uint8*)pRaster) + nXStep);
			}
			pRaster = (uint32*)(((uint8*)pRaster) + nModulo);
		}
	}
}


static void invert_line32( ServerBitmap* pcBitmap, const IRect& cClip,
                           int x1, int y1, int x2, int y2 )
{
	int nODeltaX = abs( x2 - x1 );
	int nODeltaY = abs( y2 - y1 );
	int ox1 = x1;
	int oy1 = y1;
	uint32* pRaster;
	int          nModulo = pcBitmap->BytesPerRow();

	if ( VesaDriver::ClipLine( cClip, &x1, &y1, &x2, &y2 ) == false )
		return;

	int nDeltaX = abs( x2 - x1 );
	int nDeltaY = abs( y2 - y1 );

	if ( nODeltaX > nODeltaY )
	{
		int dinc1 = nODeltaY << 1;
		int dinc2 = (nODeltaY - nODeltaX) << 1;
		int d = dinc1 - nODeltaX;

		int nYStep;

		if ( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaY*dinc2) + ((nClipDeltaX-nClipDeltaY)*dinc1));
		}

		if ( y1 > y2 )
		{
			nYStep = -nModulo;
		}
		else
		{
			nYStep = nModulo;
		}
		if ( x1 > x2 )
		{
			nYStep = -nYStep;
			pRaster = (uint32*)(pcBitmap->Bits() + x2 * 4 + nModulo * y2);
		}
		else
		{
			pRaster = (uint32*)(pcBitmap->Bits() + x1 * 4 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaX ; ++i )
		{
			rgb_color sColor = RGB32_TO_COL( *pRaster );
			sColor.red   = 255 - sColor.red;
			sColor.green = 255 - sColor.green;
			sColor.blue  = 255 - sColor.blue;
			*pRaster = COL_TO_RGB32( sColor );
			if ( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = (uint32*)(((uint8*)pRaster) + nYStep);
			}
			pRaster++;
		}
	}
	else
	{
		int dinc1 = nODeltaX << 1;
		int d = dinc1 - nODeltaY;
		int dinc2 = (nODeltaX - nODeltaY) << 1;
		int nXStep;

		if ( ox1 != x1 || oy1 != y1 )
		{
			int nClipDeltaX = abs(x1 - ox1);
			int nClipDeltaY = abs(y1 - oy1);
			d += ((nClipDeltaX*dinc2) + ((nClipDeltaY-nClipDeltaX)*dinc1));
		}


		if ( x1 > x2 )
		{
			nXStep = -sizeof(uint32);
		}
		else
		{
			nXStep = sizeof(uint32);
		}
		if ( y1 > y2 )
		{
			nXStep = -nXStep;
			pRaster = (uint32*)(pcBitmap->Bits() + x2 * 4 + nModulo * y2);
		}
		else
		{
			pRaster = (uint32*)(pcBitmap->Bits() + x1 * 4 + nModulo * y1);
		}

		for ( int i = 0 ; i <= nDeltaY ; ++i )
		{
			rgb_color sColor = RGB32_TO_COL( *pRaster );
			sColor.red   = 255 - sColor.red;
			sColor.green = 255 - sColor.green;
			sColor.blue  = 255 - sColor.blue;
			*pRaster = COL_TO_RGB32( sColor );

			if ( d < 0 )
			{
				d += dinc1;
			}
			else
			{
				d += dinc2;
				pRaster = (uint32*)(((uint8*)pRaster) + nXStep);
			}
			pRaster = (uint32*)(((uint8*)pRaster) + nModulo);
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void Blit( uint8 *Src, uint8* Dst, int SMod, int DMod, int W, int H, bool Rev )
{
	int          i;
	int           X,Y;
	uint32* LSrc;
	uint32* LDst;

	if (Rev)
	{
		for ( Y = 0 ; Y < H ; Y++ )
		{
			for( X = 0 ; (X < W) && ((uint32(Src - 3)) & 3) ; X++ ) {
				*Dst-- = *Src--;
			}

			LSrc=(uint32*)(((uint32)Src)-3);
			LDst=(uint32*)(((uint32)Dst)-3);

			i = (W - X) / 4;

			X += i * 4;

			for ( ; i ; i-- ) {
				*LDst--=*LSrc--;
			}

			Src=(uint8*)(((uint32)LSrc)+3);
			Dst=(uint8*)(((uint32)LDst)+3);

			for ( ; X < W ; X++ ) {
				*Dst--=*Src--;
			}

			Dst-= (int32)DMod;
			Src-= (int32)SMod;
		}
	}
	else
	{
		for ( Y = 0 ; Y < H ; Y++ )
		{
			for ( X = 0 ; (X < W) && (((uint32)Src) & 3) ; ++X ) {
				*Dst++ = *Src++;
			}

			LSrc = (uint32*) Src;
			LDst = (uint32*) Dst;

			i = (W - X) / 4;

			X += i * 4;

			for( ; i ; i-- ) {
				*LDst++ = *LSrc++;
			}

			Src        =        (uint8*) LSrc;
			Dst        =        (uint8*) LDst;

			for ( ; X < W ; X++ ) {
				*Dst++=*Src++;
			}

			Dst += DMod;
			Src += SMod;
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void BitBlit( ServerBitmap *sbm, ServerBitmap *dbm,
                            int sx, int sy, int dx, int dy, int w, int h )
{
	int        Smod,Dmod;
	int        BytesPerPix = 1;

	int        InPtr,OutPtr;

	int nBitsPerPix = BitsPerPixel( dbm->ColorSpace() );

	if ( nBitsPerPix == 15 ) {
		BytesPerPix = 2;
	} else {
		BytesPerPix = nBitsPerPix / 8;
	}

	sx *= BytesPerPix;
	dx *= BytesPerPix;
	w  *= BytesPerPix;

	if ( sx >= dx )
	{
		if ( sy >= dy )
		{
			Smod = sbm->BytesPerRow() - w;
			Dmod = dbm->BytesPerRow() - w;
			InPtr  = sy * sbm->BytesPerRow() + sx;
			OutPtr = dy * dbm->BytesPerRow() + dx;

			Blit( sbm->Bits() + InPtr, dbm->Bits() + OutPtr, Smod, Dmod, w, h, false );
		}
		else
		{
			Smod        =-sbm->BytesPerRow() - w;
			Dmod        =-dbm->BytesPerRow() - w;
			InPtr       = ((sy+h-1)*sbm->BytesPerRow())+sx;
			OutPtr      = ((dy+h-1)*dbm->BytesPerRow())+dx;

			Blit( sbm->Bits() + InPtr, dbm->Bits() + OutPtr, Smod, Dmod, w, h, false );
		}
	}
	else
	{
		if ( sy > dy )
		{
			Smod=-(sbm->BytesPerRow() + w);
			Dmod=-(dbm->BytesPerRow() + w);
			InPtr                = (sy*sbm->BytesPerRow())+sx+w-1;
			OutPtr        = (dy*dbm->BytesPerRow())+dx+w-1;
			Blit( sbm->Bits() + InPtr, dbm->Bits() + OutPtr, Smod, Dmod, w, h, true );
		}
		else
		{
			Smod = sbm->BytesPerRow() - w;
			Dmod = dbm->BytesPerRow() - w;
			InPtr                = (sy + h - 1) * sbm->BytesPerRow() + sx + w - 1;
			OutPtr        = (dy + h - 1) * dbm->BytesPerRow() + dx + w - 1;
			Blit( sbm->Bits() + InPtr, dbm->Bits() + OutPtr, Smod, Dmod, w, h, true );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------


static inline void blit_convert_copy( ServerBitmap* pcDst, ServerBitmap* pcSrc,
                                      const IRect& cSrcRect, const IPoint& cDstPos )
{
	switch( (int)pcSrc->ColorSpace() )
	{
		case B_COLOR_8_BIT:
		{
			uint8* pSrc = RAS_OFFSET8( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );

			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width() + 1);

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB15:
				case B_RGBA15:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );

					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB15( g_asDefaultPallette[*pSrc++] );
						}
						pSrc += nSrcModulo;
						pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB16:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );

					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB16( g_asDefaultPallette[*pSrc++] );
						}
						pSrc += nSrcModulo;
						pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB32:
				case B_RGBA32:
				{
					uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB32( g_asDefaultPallette[*pSrc++] );
						}
						pSrc += nSrcModulo;
						pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
			}
			break;
		}
		case B_RGB15:
		case B_RGBA15:
		{
			uint16* pSrc = RAS_OFFSET16( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );
			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1) * 2;

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB16:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB16( RGB15_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB32:
				case B_RGBA32:
				{
					uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB32( RGB15_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
			}
			break;
		}
		case B_RGB16:
		{
			uint16* pSrc = RAS_OFFSET16( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );
			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1) * 2;

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB15:
				case B_RGBA15:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB15( RGB16_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB32:
				case B_RGBA32:
				{
					uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB32( RGB16_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
			}
			break;
		}
		case B_RGB32:
		case B_RGBA32:
		{
			uint32* pSrc = RAS_OFFSET32( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );
			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1) * 4;

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB16:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB16( RGB32_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB15:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB15( RGB32_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
			}
			break;
		}
		default:
			STRACE(( "blit_convert_copy() unknown src color space %d\n", pcSrc->ColorSpace() ));
			break;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void blit_convert_over( ServerBitmap* pcDst, ServerBitmap* pcSrc,
                                      const IRect& cSrcRect, const IPoint& cDstPos )
{
	switch( (int)pcSrc->ColorSpace() )
	{
		case B_COLOR_8_BIT:
		{
			uint8* pSrc = RAS_OFFSET8( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );

			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1);

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB15:
				case B_RGBA15:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );

					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							int nPix = *pSrc++;
							if ( nPix != TRANSPARENT_CMAP8 ) {
								*pDst = COL_TO_RGB15( g_asDefaultPallette[nPix] );
							}
							pDst++;
						}
						pSrc += nSrcModulo;
						pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB16:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							int nPix = *pSrc++;
							if ( nPix != TRANSPARENT_CMAP8 ) {
								*pDst = COL_TO_RGB16( g_asDefaultPallette[nPix] );
							}
							pDst++;
						}
						pSrc += nSrcModulo;
						pDst  = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB32:
				case B_RGBA32:
				{
					uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							int nPix = *pSrc++;
							if ( nPix != TRANSPARENT_CMAP8 ) {
								*pDst = COL_TO_RGB32( g_asDefaultPallette[nPix] );
							}
							pDst++;
						}
						pSrc += nSrcModulo;
						pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				default:
					STRACE(( "blit_convert_over() unknown dst colorspace for 8 bit src %d\n", pcDst->ColorSpace() ));
					break;
			}
			break;
		}
		case B_RGB15:
		case B_RGBA15:
		{
			uint16* pSrc = RAS_OFFSET16( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );
			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1) * 2;

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB16:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB16( RGB15_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB32:
				case B_RGBA32:
				{
					uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );

					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB32( RGB15_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
			}
			break;
		}
		case B_RGB16:
		{
			uint16* pSrc = RAS_OFFSET16( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );
			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1) * 2;

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB15:
				case B_RGBA15:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB15( RGB16_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB32:
				case B_RGBA32:
				{
					uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							*pDst++ = COL_TO_RGB32( RGB16_TO_COL( *pSrc++ ) );
						}
						pSrc = (uint16*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
			}
			break;
		}
		case B_RGB32:
		case B_RGBA32:
		{
			uint32* pSrc = RAS_OFFSET32( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );
			int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1) * 4;

			switch( (int)pcDst->ColorSpace() )
			{
				case B_RGB16:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							uint32 nPix = *pSrc++;
							if ( nPix != 0xffffffff ) {
								*pDst = COL_TO_RGB16( RGB32_TO_COL( nPix ) );
							}
							pDst++;
						}
						pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB15:
				{
					uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							uint32 nPix = *pSrc++;
							if ( nPix != 0xffffffff ) {
								*pDst = COL_TO_RGB15( RGB32_TO_COL( nPix ) );
							}
							pDst++;
						}
						pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
				case B_RGB32:
				{
					uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
					int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

					for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
						for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
							uint32 nPix = *pSrc++;
							if ( nPix != 0xffffffff ) {
								*pDst = nPix;
							}
							pDst++;
						}
						pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
						pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
					}
					break;
				}
			}
			break;
		}
		default:
			STRACE(( "blit_convert_over() unknown src color space %d\n", pcSrc->ColorSpace() ));
			break;
	}
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static inline void blit_convert_alpha( ServerBitmap* pcDst, ServerBitmap* pcSrc,
                                       const IRect& cSrcRect, const IPoint& cDstPos )
{
	uint32* pSrc = RAS_OFFSET32( pcSrc->Bits(), cSrcRect.left, cSrcRect.top, pcSrc->BytesPerRow() );
	int nSrcModulo = pcSrc->BytesPerRow() - (cSrcRect.Width()+1) * 4;

	switch( (int)pcDst->ColorSpace() )
	{
		case B_RGB16:
		{
			uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
			int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

			for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
				for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
					rgb_color sSrcColor = RGB32_TO_COL( *pSrc++ );

					int nAlpha = sSrcColor.alpha;
					if ( nAlpha == 0xff ) {
						*pDst = COL_TO_RGB16( sSrcColor );
					} else if ( nAlpha != 0x00 ) {
						rgb_color sDstColor = RGB16_TO_COL( *pDst );
						rgb_color color = {
								sDstColor.red   * (256-nAlpha) / 256 + sSrcColor.red   * nAlpha / 256,
								sDstColor.green * (256-nAlpha) / 256 + sSrcColor.green * nAlpha / 256,
								sDstColor.blue  * (256-nAlpha) / 256 + sSrcColor.blue  * nAlpha / 256,
								0 };
						*pDst = COL_TO_RGB16(color);
					}
					pDst++;
				}
				pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
				pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
			}
			break;
		}
		case B_RGB15:
		{
			uint16* pDst = RAS_OFFSET16( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
			int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 2;

			for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
				for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
					rgb_color sSrcColor = RGB32_TO_COL( *pSrc++ );

					int nAlpha = sSrcColor.alpha;
					if ( nAlpha == 0xff ) {
						*pDst = COL_TO_RGB15( sSrcColor );
					} else if ( nAlpha != 0x00 ) {
						rgb_color sDstColor = RGB15_TO_COL( *pDst );
						rgb_color color = {
								sDstColor.red   * (256-nAlpha) / 256 + sSrcColor.red   * nAlpha / 256,
								sDstColor.green * (256-nAlpha) / 256 + sSrcColor.green * nAlpha / 256,
								sDstColor.blue  * (256-nAlpha) / 256 + sSrcColor.blue  * nAlpha / 256,
								0 };
						*pDst = COL_TO_RGB15(color);
					}
					pDst++;
				}
				pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
				pDst = (uint16*)(((uint8*)pDst) + nDstModulo);
			}
			break;
		}
		case B_RGB32:
		{
			uint32* pDst = RAS_OFFSET32( pcDst->Bits(), cDstPos.x, cDstPos.y, pcDst->BytesPerRow() );
			int nDstModulo = pcDst->BytesPerRow() - (cSrcRect.Width()+1) * 4;

			for ( int y = cSrcRect.top ; y <= cSrcRect.bottom ; ++y ) {
				for ( int x = cSrcRect.left ; x <= cSrcRect.right ; ++x ) {
					rgb_color sSrcColor = RGB32_TO_COL( *pSrc++ );

					int nAlpha = sSrcColor.alpha;
					if ( nAlpha == 0xff ) {
						*pDst = COL_TO_RGB32( sSrcColor );
					} else if ( nAlpha != 0x00 ) {
						rgb_color sDstColor = RGB32_TO_COL( *pDst );
						rgb_color color = {
								sDstColor.red   * (256-nAlpha) / 256 + sSrcColor.red   * nAlpha / 256,
								sDstColor.green * (256-nAlpha) / 256 + sSrcColor.green * nAlpha / 256,
								sDstColor.blue  * (256-nAlpha) / 256 + sSrcColor.blue  * nAlpha / 256,
								0 };
						*pDst = COL_TO_RGB32(color);
					}
					pDst++;
				}
				pSrc = (uint32*)(((uint8*)pSrc) + nSrcModulo);
				pDst = (uint32*)(((uint8*)pDst) + nDstModulo);
			}
			break;
		}
	}
}


/*!
	\brief Changes the cursor.
	\param cursor The new cursor. Guaranteed to be non-NULL.

	The driver does not take ownership of the given cursor. Subclasses should make
	a copy of the cursor passed to it. The default version of this function hides the
	cursor, replaces it, and shows the cursor if previously visible.
*/
void VesaDriver::SetCursor(ServerCursor *cursor)
{
	// Use the default method to set the cursor bitmap
	DisplayDriver::SetCursor(cursor);
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
}

void VesaDriver::FillBlit8( uint8* pDst, int nMod, int W,int H, int nColor )
{
	int X,Y;

	for ( Y = 0 ; Y < H ; Y++ )
	{
		for ( X = 0 ; X < W ; X++ )
		{
			*pDst++ = nColor;
		}
		pDst += nMod;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void VesaDriver::FillBlit16( uint16 *pDst, int nMod, int W, int H, uint32 nColor )
{
	int X,Y;

	for ( Y = 0 ; Y < H ; Y++ )
	{
		for ( X = 0 ; X < W ; X++ )
		{
			*pDst++ = nColor;
		}
		pDst += nMod;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void VesaDriver::FillBlit24( uint8* pDst, int nMod, int W, int H, uint32 nColor )
{
	int X,Y;

	for ( Y = 0 ; Y < H ; Y++ )
	{
		for ( X = 0 ; X < W ; X++ )
		{
			*pDst++ = nColor & 0xff;
			*((uint16*)pDst) = nColor >> 8;
			pDst += 2;
		}
		pDst += nMod;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void VesaDriver::FillBlit32( uint32 *pDst, int nMod, int W, int H, uint32 nColor )
{
	int X,Y;

	for ( Y = 0 ; Y < H ; Y++ )
	{
		for ( X = 0 ; X < W ; X++ )
		{
			*pDst++ = nColor;
		}
		pDst += nMod;
	}
}

bool VesaDriver::ClipLine( const IRect& cRect, int* x1, int* y1, int* x2, int* y2 )
{
	bool point_1 = false;
	bool point_2 = false;  // tracks if each end point is visible or invisible

	bool clip_always = false;           // used for clipping override

	int xi=0,yi=0;                     // point of intersection

	bool right_edge  = false;              // which edges are the endpoints beyond
	bool left_edge   = false;
	bool top_edge    = false;
	bool bottom_edge = false;


	bool success = false;               // was there a successfull clipping

	float dx,dy;                   // used to holds slope deltas

// test if line is completely visible

	if ( (*x1>=cRect.left) && (*x1<=cRect.right) && (*y1>=cRect.top) && (*y1<=cRect.bottom) ) {
		point_1 = true;
	}

	if ( (*x2 >= cRect.left) && (*x2 <= cRect.right) && (*y2 >= cRect.top) && (*y2 <= cRect.bottom) ) {
		point_2 = true;
	}


	// test endpoints
	if (point_1 && point_2) {
		return( true );
	}

	// test if line is completely invisible
	if (point_1==false && point_2==false)
	{
		// must test to see if each endpoint is on the same side of one of
		// the bounding planes created by each clipping region boundary

		if ( ( (*x1<cRect.left) && (*x2<cRect.left) ) ||  // to the left
			( (*x1>cRect.right) && (*x2>cRect.right) ) ||  // to the right
			( (*y1<cRect.top) && (*y2<cRect.top) ) ||  // above
			( (*y1>cRect.bottom) && (*y2>cRect.bottom) ) ) { // below
			return(0); // the entire line is otside the rectangle
		}

		// if we got here we have the special case where the line cuts into and
		// out of the clipping region
		clip_always = true;
	}

	// take care of case where either endpoint is in clipping region
	if ( point_1 || clip_always )
	{
		dx = *x2 - *x1; // compute deltas
		dy = *y2 - *y1;

		// compute what boundary line need to be clipped against
		if (*x2 > cRect.right) {
			// flag right edge
			right_edge = true;

			// compute intersection with right edge
			if (dx!=0)
				yi = (int)(.5 + (dy/dx) * (cRect.right - *x1) + *y1);
			else
				yi = -1;  // invalidate intersection
		} else if (*x2 < cRect.left) {
			// flag left edge
			left_edge = true;

			// compute intersection with left edge
			if (dx!=0) {
				yi = (int)(.5 + (dy/dx) * (cRect.left - *x1) + *y1);
			} else {
				yi = -1;  // invalidate intersection
			}
		}

		// horizontal intersections
		if (*y2 > cRect.bottom) {
			bottom_edge = true; // flag bottom edge

			// compute intersection with right edge
			if (dy!=0) {
				xi = (int)(.5 + (dx/dy) * (cRect.bottom - *y1) + *x1);
			} else {
				xi = -1;  // invalidate inntersection
			}
		} else if (*y2 < cRect.top) {
			// flag top edge
			top_edge = true;

			// compute intersection with top edge
			if (dy!=0) {
				xi = (int)(.5 + (dx/dy) * (cRect.top - *y1) + *x1);
			} else {
				xi = -1;  // invalidate intersection
			}
		}

		// now we know where the line passed thru
		// compute which edge is the proper intersection

		if ( right_edge == true && (yi >= cRect.top && yi <= cRect.bottom) )
		{
			*x2 = cRect.right;
			*y2 = yi;
			success = true;
		} else if (left_edge && (yi>=cRect.top && yi<=cRect.bottom) ) {
			*x2 = cRect.left;
			*y2 = yi;

			success = true;
		}

		if ( bottom_edge == true && (xi >= cRect.left && xi <= cRect.right) ) {
			*x2 = xi;
			*y2 = cRect.bottom;
			success = true;
		} else if (top_edge && (xi>=cRect.left && xi<=cRect.right) ) {
			*x2 = xi;
			*y2 = cRect.top;
			success = true;
		}
	} // end if point_1 is visible

	// reset edge flags
	right_edge = left_edge = top_edge = bottom_edge = false;

	// test second endpoint
	if ( point_2 || clip_always )
	{
		dx = *x1 - *x2; // compute deltas
		dy = *y1 - *y2;

		// compute what boundary line need to be clipped against
		if ( *x1 > cRect.right ) {

			// flag right edge
			right_edge = true;

			// compute intersection with right edge
			if (dx!=0) {
				yi = (int)(.5 + (dy/dx) * (cRect.right - *x2) + *y2);
			} else {
				yi = -1;  // invalidate inntersection
			}
		} else if (*x1 < cRect.left) {
			left_edge = true; // flag left edge

			// compute intersection with left edge
			if (dx!=0) {
				yi = (int)(.5 + (dy/dx) * (cRect.left - *x2) + *y2);
			} else {
				yi = -1;  // invalidate intersection
			}
		}

		// horizontal intersections
		if (*y1 > cRect.bottom) {
			bottom_edge = true; // flag bottom edge

			// compute intersection with right edge
			if (dy!=0) {
				xi = (int)(.5 + (dx/dy) * (cRect.bottom - *y2) + *x2);
			} else {
				xi = -1;  // invalidate inntersection
			}
		} else if (*y1 < cRect.top) {
			top_edge = true; // flag top edge

			// compute intersection with top edge
			if (dy!=0) {
				xi = (int)(.5 + (dx/dy) * (cRect.top - *y2) + *x2);
			} else {
				xi = -1;  // invalidate inntersection
			}
		}

		// now we know where the line passed thru
		// compute which edge is the proper intersection
		if ( right_edge && (yi >= cRect.top && yi <= cRect.bottom) ) {
			*x1 = cRect.right;
			*y1 = yi;
			success = true;
		} else if ( left_edge && (yi >= cRect.top && yi <= cRect.bottom) ) {
			*x1 = cRect.left;
			*y1 = yi;
			success = true;
		}

		if (bottom_edge && (xi >= cRect.left && xi <= cRect.right) ) {
			*x1 = xi;
			*y1 = cRect.bottom;
			success = true;
		} else if (top_edge==1 && (xi>=cRect.left && xi<=cRect.right) ) {
			*x1 = xi;
			*y1 = cRect.top;

			success = true;
		}
	} // end if point_2 is visible

	return(success);
}
