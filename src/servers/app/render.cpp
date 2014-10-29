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


#include <stdio.h>

#include "DisplayDriver.h"
#include "AppServer.h"
#include "Layer.h"
#include "ServerFont.h"
#include <ServerBitmap.h>
#include "sprite.h"
#include "ServerWindow.h"
#include "clipping.h"
#include "FontServer.h"

#include <macros.h>

#include <Rect.h>

#include <ServerProtocol.h>


static rgb_color g_asPens[128];

static rgb_color GetStdCol( int i )
{
	static uint8        DefaultPalette[] =
	{
		0x00, 0x60, 0x6b,
		0x00, 0x00, 0x00,
		0xff, 0xff, 0xff,
		0x66, 0x88, 0xbb,

		0x8f, 0x8f, 0x8f,
		0xc6, 0xc6, 0xc6,
		0xbb, 0xaa, 0x99,
		0xff, 0xbb, 0xaa,

		0xff, 0x00, 0x00,
		0x78, 0x78, 0x78,
		0xb4, 0xb4, 0xb4,
		0xdc, 0xdc, 0xdc,

		0x55, 0x55, 0xff,
		0x99, 0x22, 0xff,
		0x00, 0xff, 0x88,
		0xcc, 0xcc, 0xcc,

		0x00, 0x00, 0x00,
		0xe0, 0x40, 0x40,
		0x00, 0x00, 0x00,
		0xe0, 0xe0, 0xc0,
		0x44, 0x44, 0x44,
		0x55, 0x55, 0x55,
		0x66, 0x66, 0x66,
		0x77, 0x77, 0x77,
		0x88, 0x88, 0x88,
		0x99, 0x99, 0x99,
		0xaa, 0xaa, 0xaa,
		0xbb, 0xbb, 0xbb,
		0xcc, 0xcc, 0xcc,
		0xdd, 0xdd, 0xdd,
		0xee, 0xee, 0xee,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x88, 0x88, 0x88,
		0xff, 0xff, 0xff,
		0xcc, 0xcc, 0xcc,
		0x44, 0x44, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x88, 0xff, 0xff,
		0x44, 0x88, 0x88,
		0xcc, 0xff, 0xff,
		0x66, 0xcc, 0xcc,
		0x22, 0x44, 0x44,
		0xaa, 0xff, 0xff,
		0xee, 0xff, 0xff,
		0xcc, 0xff, 0xff,
		0x66, 0x88, 0x88,
		0xff, 0xff, 0xff,
		0x99, 0xcc, 0xcc,
		0x33, 0x44, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x44, 0xff, 0xff,
		0x22, 0x88, 0x88,
		0x66, 0xff, 0xff,
		0x33, 0xcc, 0xcc,
		0x11, 0x44, 0x44,
		0x55, 0xff, 0xff,
		0x77, 0xff, 0xff,
		0xff, 0x88, 0xff,
		0x88, 0x44, 0x88,
		0xff, 0xcc, 0xff,
		0xcc, 0x66, 0xcc,
		0x44, 0x22, 0x44,
		0xff, 0xaa, 0xff,
		0xff, 0xee, 0xff,
		0x88, 0x88, 0xff,
		0x44, 0x44, 0x88,
		0xcc, 0xcc, 0xff,
		0x66, 0x66, 0xcc,
		0x22, 0x22, 0x44,
		0xaa, 0xaa, 0xff,
		0xee, 0xee, 0xff,
		0xcc, 0x88, 0xff,
		0x66, 0x44, 0x88,
		0xff, 0xcc, 0xff,
		0x99, 0x66, 0xcc,
		0x33, 0x22, 0x44,
		0xff, 0xaa, 0xff,
		0xff, 0xee, 0xff,
		0x44, 0x88, 0xff,
		0x22, 0x44, 0x88,
		0x66, 0xcc, 0xff,
		0x33, 0x66, 0xcc,
		0x11, 0x22, 0x44,
		0x55, 0xaa, 0xff,
		0x77, 0xee, 0xff,
		0xff, 0xcc, 0xff,
		0x88, 0x66, 0x88,
		0xff, 0xff, 0xff,
		0xcc, 0x99, 0xcc,
		0x44, 0x33, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x88, 0xcc, 0xff,
		0x44, 0x66, 0x88,
		0xcc, 0xff, 0xff,
		0x66, 0x99, 0xcc,
		0x22, 0x33, 0x44,
		0xaa, 0xff, 0xff,
		0xee, 0xff, 0xff,
		0xcc, 0xcc, 0xff,
		0x66, 0x66, 0x88,
		0xff, 0xff, 0xff,
		0x99, 0x99, 0xcc,
		0x33, 0x33, 0x44,
		0xff, 0xff, 0xff,
		0xff, 0xff, 0xff,
		0x44, 0xcc, 0xff,
		0x22, 0x66, 0x88,
		0x66, 0xff, 0xff,
		0x33, 0x99, 0xcc,
		0x11, 0x33, 0x44,
		0x55, 0xff, 0xff,
		0x77, 0xff, 0xff,
		0xff, 0x44, 0xff,
		0x88, 0x22, 0x88,
		0xff, 0x66, 0xff,
		0xcc, 0x33, 0xcc,
		0x44, 0x11, 0x44,
		0xff, 0x55, 0xff,
		0xff, 0x77, 0xff,
		0x88, 0x44, 0xff,
		0x44, 0x22, 0x88,
		0xcc, 0x66, 0xff,
		0x66, 0x33, 0xcc,
		0x22, 0x11, 0x44,
		0xaa, 0x55, 0xff,
		0xee, 0x77, 0xff,
		0xcc, 0x44, 0xff,
		0x66, 0x22, 0x88,
		0xff, 0x66, 0xff,
		0x99, 0x33, 0xcc,
		0x33, 0x11, 0x44,
		0xff, 0x55, 0xff,
		0xff, 0x77, 0xff,
		0x44, 0x44, 0xff,
		0x22, 0x22, 0x88,
		0x66, 0x66, 0xff,
		0x33, 0x33, 0xcc,
		0x11, 0x11, 0x44,
		0x55, 0x55, 0xff,
		0x77, 0x77, 0xff,
		0xff, 0xff, 0x88,
		0x88, 0x88, 0x44,
		0xff, 0xff, 0xcc,
		0xcc, 0xcc, 0x66,
		0x44, 0x44, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x88, 0xff, 0x88,
		0x44, 0x88, 0x44,
		0xcc, 0xff, 0xcc,
		0x66, 0xcc, 0x66,
		0x22, 0x44, 0x22,
		0xaa, 0xff, 0xaa,
		0xee, 0xff, 0xee,
		0xcc, 0xff, 0x88,
		0x66, 0x88, 0x44,
		0xff, 0xff, 0xcc,
		0x99, 0xcc, 0x66,
		0x33, 0x44, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x44, 0xff, 0x88,
		0x22, 0x88, 0x44,
		0x66, 0xff, 0xcc,
		0x33, 0xcc, 0x66,
		0x11, 0x44, 0x22,
		0x55, 0xff, 0xaa,
		0x77, 0xff, 0xee,
		0xff, 0x88, 0x88,
		0x88, 0x44, 0x44,
		0xff, 0xcc, 0xcc,
		0xcc, 0x66, 0x66,
		0x44, 0x22, 0x22,
		0xff, 0xaa, 0xaa,
		0xff, 0xee, 0xee,
		0x88, 0x88, 0x88,
		0x44, 0x44, 0x44,
		0xcc, 0xcc, 0xcc,
		0x66, 0x66, 0x66,
		0x22, 0x22, 0x22,
		0xaa, 0xaa, 0xaa,
		0xee, 0xee, 0xee,
		0xcc, 0x88, 0x88,
		0x66, 0x44, 0x44,
		0xff, 0xcc, 0xcc,
		0x99, 0x66, 0x66,
		0x33, 0x22, 0x22,
		0xff, 0xaa, 0xaa,
		0xff, 0xee, 0xee,
		0x44, 0x88, 0x88,
		0x22, 0x44, 0x44,
		0x66, 0xcc, 0xcc,
		0x33, 0x66, 0x66,
		0x11, 0x22, 0x22,
		0x55, 0xaa, 0xaa,
		0x77, 0xee, 0xee,
		0xff, 0xcc, 0x88,
		0x88, 0x66, 0x44,
		0xff, 0xff, 0xcc,
		0xcc, 0x99, 0x66,
		0x44, 0x33, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x88, 0xcc, 0x88,
		0x44, 0x66, 0x44,
		0xcc, 0xff, 0xcc,
		0x66, 0x99, 0x66,
		0x22, 0x33, 0x22,
		0xaa, 0xff, 0xaa,
		0xee, 0xff, 0xee,
		0xcc, 0xcc, 0x88,
		0x66, 0x66, 0x44,
		0xff, 0xff, 0xcc,
		0x99, 0x99, 0x66,
		0x33, 0x33, 0x22,
		0xff, 0xff, 0xaa,
		0xff, 0xff, 0xee,
		0x44, 0xcc, 0x88,
		0x22, 0x66, 0x44,
		0x66, 0xff, 0xcc,
		0x33, 0x99, 0x66,
		0x11, 0x33, 0x22,
		0x55, 0xff, 0xaa,
		0x77, 0xff, 0xee,
		0xff, 0x44, 0x88,
		0x88, 0x22, 0x44,
		0xff, 0x66, 0xcc,
		0xcc, 0x33, 0x66,
		0x44, 0x11, 0x22,
		0xff, 0x55, 0xaa,
		0xff, 0x77, 0xee,
		0x88, 0x44, 0x88,
		0x44, 0x22, 0x44,
		0xcc, 0x66, 0xcc,
		0x66, 0x33, 0x66,
		0x22, 0x11, 0x22,
		0xaa, 0x55, 0xaa,
		0xee, 0x77, 0xee,
		0xcc, 0x44, 0x88,
		0x66, 0x22, 0x44,
		0xff, 0x66, 0xcc,
		0x99, 0x33, 0x66,
		0x33, 0x11, 0x22,
		0xff, 0x55, 0xaa,
		0xff, 0x77, 0xee,
		0x44, 0x44, 0x88,
		0xbb, 0xbb, 0xbb,
		0x99, 0x99, 0x99,
		0x8f, 0x8f, 0x8f,
		0xc6, 0xc6, 0xc6,
		0xbb, 0xaa, 0x99,
		0x4d, 0x75, 0x44
	};

	rgb_color        sColor;
	sColor.red   = DefaultPalette[ i * 3 + 0 ];
	sColor.green = DefaultPalette[ i * 3 + 1 ];
	sColor.blue  = DefaultPalette[ i * 3 + 2 ];
	sColor.alpha = 0;

	return( sColor );
}



rgb_color GetDefaultColor( int nIndex )
{
	static bool bFirst = true;

	if ( bFirst )
	{
		g_asPens[ PEN_BACKGROUND ]   = GetStdCol( 4  );
		g_asPens[ PEN_DETAIL ]       = GetStdCol( 1  );
		g_asPens[ PEN_SHINE ]        = GetStdCol( 2  );
		g_asPens[ PEN_SHADOW ]       = GetStdCol( 1  );
		g_asPens[ PEN_BRIGHT ]       = GetStdCol( 11 );
		g_asPens[ PEN_DARK ]         = GetStdCol( 9  );
		g_asPens[ PEN_WINTITLE ]     = GetStdCol( 9  );
		g_asPens[ PEN_WINBORDER ]    = GetStdCol( 10 );
		g_asPens[ PEN_SELWINTITLE ]  = GetStdCol( 3  );
		g_asPens[ PEN_SELWINBORDER ] = GetStdCol( 10 );
		g_asPens[ PEN_WINDOWTEXT ]   = GetStdCol( 1  );
		g_asPens[ PEN_SELWNDTEXT ]   = GetStdCol( 2  );
		g_asPens[ PEN_WINCLIENT ]    = GetStdCol( 5  );
		g_asPens[ PEN_GADGETFILL ]   = GetStdCol( 4  );
		g_asPens[ PEN_SELGADGETFILL ]= GetStdCol( 9  );
		g_asPens[ PEN_GADGETTEXT ]   = GetStdCol( 2  );
		g_asPens[ PEN_SELGADGETTEXT ]= GetStdCol( 1  );
		bFirst = false;
	}
	return( g_asPens[ nIndex ] );
}


void Layer::DrawFrame( const BRect& cRect, uint32 nStyle )
{
	bool        Sunken = false;

	if ( ((nStyle & FRAME_RAISED) == 0) && (nStyle & (FRAME_RECESSED)) )
	{
		Sunken = true;
	}

	if ( nStyle & FRAME_FLAT )
	{
	}
	else
	{
		rgb_color sShinePen  = GetDefaultColor( PEN_SHINE );
		rgb_color sShadowPen = GetDefaultColor( PEN_SHADOW );

		if ( (nStyle & FRAME_TRANSPARENT) == 0 )
		{
			FillRect( BRect( cRect.left + 2, cRect.top + 2, cRect.right - 2, cRect.bottom - 2), fLayerData->viewcolor.GetColor32() );
		}

		SetHighColor( (Sunken) ? sShadowPen : sShinePen );

		MovePenTo( cRect.left, cRect.bottom );
		DrawLine( BPoint( cRect.left, cRect.top ) );
		DrawLine( BPoint( cRect.right, cRect.top ) );

		SetHighColor( (Sunken) ? sShinePen : sShadowPen );

		DrawLine( BPoint( cRect.right, cRect.bottom ) );
		DrawLine( BPoint( cRect.left, cRect.bottom ) );


		if ( (nStyle & FRAME_THIN) == 0 )
		{
			if ( nStyle & FRAME_ETCHED )
			{
				SetHighColor( (Sunken) ? sShadowPen : sShinePen );

				MovePenTo( cRect.left + 1, cRect.bottom - 1 );

				DrawLine( BPoint( cRect.left + 1, cRect.top + 1 ) );
				DrawLine( BPoint( cRect.right - 1, cRect.top + 1 ) );

				SetHighColor( (Sunken) ? sShinePen : sShadowPen );

				DrawLine( BPoint( cRect.right - 1, cRect.bottom - 1 ) );
				DrawLine( BPoint( cRect.left + 1, cRect.bottom - 1 ) );
			}
			else
			{
				rgb_color        sBrightPen = GetDefaultColor( PEN_BRIGHT );
				rgb_color        sDarkPen   = GetDefaultColor( PEN_DARK );

				SetHighColor( (Sunken) ? sDarkPen : sBrightPen );

				MovePenTo( cRect.left + 1, cRect.bottom - 1 );

				DrawLine( BPoint( cRect.left + 1, cRect.top + 1 ) );
				DrawLine( BPoint( cRect.right - 1, cRect.top + 1 ) );

				SetHighColor( (Sunken) ? sBrightPen : sDarkPen );

				DrawLine( BPoint( cRect.right - 1, cRect.bottom - 1 ) );
				DrawLine( BPoint( cRect.left + 1, cRect.bottom - 1 ) );
			}
		}
		else
		{
			if ( (nStyle & FRAME_TRANSPARENT) == 0 )
			{
				FillRect( BRect( cRect.left + 1, cRect.top + 1, cRect.right - 1, cRect.bottom - 1), fLayerData->viewcolor.GetColor32() );
			}
		}
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::DrawLine( const BPoint& cToPos )
{
	if ( NULL == m_pcBitmap )
	{
		return;
	}

	BRegion* pcReg = GetRegion();

	if ( NULL == pcReg )
	{
		return;
	}

	BPoint cTopLeft = ConvertToRoot( BPoint(0,0) );
	BRect  aRect;
	BPoint cMin( fLayerData->penlocation + cTopLeft + m_cScrollOffset );
	BPoint cMax( cToPos + cTopLeft + m_cScrollOffset );

	fLayerData->penlocation = cToPos;


	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(cTopLeft))) );
	}

	if(fLayerData->clipReg)
		fLayerData->clipReg->MakeEmpty();
	else
		fLayerData->clipReg = new BRegion();

	fLayerData->clipReg->Include(pcReg);
	fLayerData->clipReg->OffsetBy(cTopLeft.x, cTopLeft.y);
	m_pcBitmap->m_pcDriver->StrokeLine(cMin, cMax, fLayerData);
	fLayerData->clipReg->MakeEmpty();

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}


static void RenderGlyph( ServerBitmap *pcBitmap,
						 Glyph* pcGlyph,
						 const IPoint& cPos,
						 const IRect& cClipRect,
						 const rgb_color& sFgColor )
{
	IRect        cBounds = pcGlyph->m_cBounds + cPos;
	IRect        cRect   = cBounds & cClipRect;

	if ( cRect.IsValid() == false )
	{
		return;
	}
	int sx = cRect.left - cBounds.left;
	int sy = cRect.top - cBounds.top;

	int nWidth  = cRect.Width()+1;
	int nHeight = cRect.Height()+1;

	int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;

	uint8*  pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

	rgb_color sCurCol;
	rgb_color sBgColor;

	if ( pcBitmap->ColorSpace() == B_RGB15 )
	{
		int     nDstModulo = pcBitmap->BytesPerRow() / 2 - nWidth;
		uint16* pDst = (uint16*)pcBitmap->Bits() + cRect.left + (cRect.top * pcBitmap->BytesPerRow() / 2);

		int nFgClut = COL_TO_RGB15( sFgColor );

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;

				if ( nAlpha > 0 ) {
					if ( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						*pDst = nFgClut;
					}
					else
					{
						int        nClut = *pDst;

						sBgColor = RGB16_TO_COL( nClut );

						sCurCol.red   = sBgColor.red   + (sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.green = sBgColor.green + (sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.blue  = sBgColor.blue  + (sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);

						*pDst = COL_TO_RGB15( sCurCol );
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	else if ( pcBitmap->ColorSpace() == B_RGB16 )
	{
		int        nDstModulo = pcBitmap->BytesPerRow() / 2 - nWidth;
		uint16* pDst = (uint16*)pcBitmap->Bits() + cRect.left + cRect.top * pcBitmap->BytesPerRow() / 2;

		int nFgClut = COL_TO_RGB16( sFgColor );

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;

				if ( nAlpha > 0 ) {
					if ( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						*pDst = nFgClut;
					}
					else
					{
						int        nClut = *pDst;

						sBgColor = RGB16_TO_COL( nClut );

						sCurCol.red   = sBgColor.red   + int(sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.green = sBgColor.green + int(sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.blue  = sBgColor.blue  + int(sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);

						*pDst = COL_TO_RGB16( sCurCol );
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	else if ( pcBitmap->ColorSpace() == B_RGB32 )
	{
		int        nDstModulo = pcBitmap->BytesPerRow() / 4 - nWidth;
		uint32* pDst = (uint32*)pcBitmap->Bits() + cRect.left + cRect.top * pcBitmap->BytesPerRow() / 4;

		int nFgClut = COL_TO_RGB32( sFgColor );

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;

				if ( nAlpha > 0 )
				{
					if ( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						*pDst = nFgClut;
					}
					else
					{
						int        nClut = *pDst;

						sBgColor = RGB32_TO_COL( nClut );

						sCurCol.red   = sBgColor.red   + (sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.green = sBgColor.green + (sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.blue  = sBgColor.blue  + (sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);

						*pDst = COL_TO_RGB32( sCurCol );
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
}


static void RenderGlyphBlend( ServerBitmap *pcBitmap,
							  Glyph* pcGlyph,
							  const IPoint& cPos,
							  const IRect& cClipRect,
							  const rgb_color& sFgColor )
{
	IRect        cBounds = pcGlyph->m_cBounds + cPos;
	IRect        cRect   = cBounds & cClipRect;

	if ( cRect.IsValid() == false )
	{
		return;
	}

	int sx = cRect.left - cBounds.left;
	int sy = cRect.top - cBounds.top;

	int nWidth  = cRect.Width()+1;
	int nHeight = cRect.Height()+1;

	int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;

	uint8*  pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

	int     nDstModulo = pcBitmap->BytesPerRow() / 4 - nWidth;
	uint32* pDst = (uint32*)pcBitmap->Bits() + cRect.left + cRect.top * pcBitmap->BytesPerRow() / 4;

	for ( int y = 0 ; y < nHeight ; ++y )
	{
		for ( int x = 0 ; x < nWidth ; ++x )
		{
			int nAlpha = *pSrc++;
			rgb_color color = { sFgColor.red, sFgColor.green, sFgColor.blue, int(sFgColor.alpha * nAlpha) / 255 };
			*pDst++ = COL_TO_RGB32( color );
		}
		pSrc += nSrcModulo;
		pDst += nDstModulo;
	}
	
	//pcBitmap->m_pcDriver->Invalidate(cClipRect.AsBRect());
}


static void RenderGlyph( ServerBitmap *pcBitmap,
						 Glyph* pcGlyph,
						 const IPoint& cPos,
						 const IRect& cClipRect,
						 const uint32* aPalette )
{
	IRect cBounds = pcGlyph->m_cBounds + cPos;
	IRect cRect   = cBounds & cClipRect;

	if ( cRect.IsValid() == false )
	{
		return;
	}

	int sx = cRect.left - cBounds.left;
	int sy = cRect.top - cBounds.top;

	int nWidth  = cRect.Width()+1;
	int nHeight = cRect.Height()+1;

	int nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;

	uint8* pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

	if ( pcBitmap->ColorSpace() == B_RGB16 || pcBitmap->ColorSpace() == B_RGB15 )
	{
		int     nDstModulo = pcBitmap->BytesPerRow() / 2 - nWidth;
		uint16* pDst = (uint16*)pcBitmap->Bits() + cRect.left + cRect.top * pcBitmap->BytesPerRow() / 2;

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;
				if ( nAlpha > 0 )
				{
					*pDst = aPalette[nAlpha];
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	else if ( pcBitmap->ColorSpace() == B_RGB32 )
	{
		int        nDstModulo = pcBitmap->BytesPerRow() / 4 - nWidth;
		uint32* pDst = (uint32*)pcBitmap->Bits() + cRect.left + cRect.top * pcBitmap->BytesPerRow() / 4;

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;

				if ( nAlpha > 0 )
				{
					*pDst = aPalette[nAlpha];
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::DrawString( const char* pzString, int nLength )
{
	if ( m_pcBitmap == NULL )
	{
		return;
	}

	if ( m_pcFont == NULL )
	{
		return;
	}

	BRegion*        pcReg = GetRegion();

	if ( NULL == pcReg )
	{
		return;
	}

	if ( fLayerData->draw_mode == B_OP_COPY && m_bFontPalletteValid == false )
	{
		m_asFontPallette[0] = fLayerData->lowcolor.GetColor32();
		m_asFontPallette[NUM_FONT_GRAYS-1] = fLayerData->highcolor.GetColor32();

		for ( int i = 1 ; i < NUM_FONT_GRAYS - 1 ; ++i )
		{
			m_asFontPallette[i].red   = fLayerData->lowcolor.GetColor32().red   + int(fLayerData->highcolor.GetColor32().red   - fLayerData->lowcolor.GetColor32().red)   * i / (NUM_FONT_GRAYS-1);
			m_asFontPallette[i].green = fLayerData->lowcolor.GetColor32().green + int(fLayerData->highcolor.GetColor32().green - fLayerData->lowcolor.GetColor32().green) * i / (NUM_FONT_GRAYS-1);
			m_asFontPallette[i].blue  = fLayerData->lowcolor.GetColor32().blue  + int(fLayerData->highcolor.GetColor32().blue  - fLayerData->lowcolor.GetColor32().blue)  * i / (NUM_FONT_GRAYS-1);
		}
		m_bFontPalletteValid = true;
	}

	if ( nLength == -1 )
	{
		nLength = strlen( pzString );
	}

	BPoint cTopLeft = ConvertToRoot( BPoint(0,0) );
	IPoint cPos( fLayerData->penlocation + cTopLeft + m_cScrollOffset );
	int           i;

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(cTopLeft))) );
	}

	if ( fLayerData->draw_mode == B_OP_COPY )
	{
		uint32 anPallette[NUM_FONT_GRAYS];

		switch( m_pcBitmap->ColorSpace() )
		{
			case B_RGB15:
				for ( i = 1 ; i < NUM_FONT_GRAYS ; ++i )
				{
					anPallette[i] = COL_TO_RGB15( m_asFontPallette[i] );
				}
				break;

			case B_RGB16:
				for ( i = 1 ; i < NUM_FONT_GRAYS ; ++i )
				{
					anPallette[i] = COL_TO_RGB16( m_asFontPallette[i] );
				}
				break;

			case B_RGB32:
				for ( i = 1 ; i < NUM_FONT_GRAYS ; ++i )
				{
					anPallette[i] = COL_TO_RGB32( m_asFontPallette[i] );
				}
				break;

			default:
				printf( "Layer::DrawString() unknown colorspace %d\n", m_pcBitmap->ColorSpace() );
				break;
		}
		if ( FontServer::Lock() )
		{
			while ( nLength > 0 )
			{
				int nCharLen = utf8_char_length( *pzString );
				if ( nCharLen > nLength )
				{
					break;
				}
				Glyph* pcGlyph = m_pcFont->GetGlyph( utf8_to_unicode( pzString ) );
				pzString += nCharLen;
				nLength  -= nCharLen;
				if ( pcGlyph == NULL )
				{
					printf( "Error: Layer::DrawString() failed to load glyph\n" );
					continue;
				}

				BRect clipRect;

				ENUMBRECTS((*pcReg), clipRect)
				{
					clipRect.OffsetBy(cTopLeft);
					RenderGlyph( m_pcBitmap, pcGlyph, cPos,
								IRect(clipRect), anPallette );
				}
				cPos.x += pcGlyph->m_nAdvance;
				fLayerData->penlocation.x += pcGlyph->m_nAdvance;
			}
			m_pcBitmap->m_pcDriver->Invalidate(pcReg->Frame().OffsetByCopy(cTopLeft));
			FontServer::Unlock();
		}
	}
	else if ( fLayerData->draw_mode == B_OP_BLEND && m_pcBitmap->ColorSpace() == B_RGB32 )
	{
		if ( FontServer::Lock() )
		{
			while ( nLength > 0 )
			{
				int nCharLen = utf8_char_length( *pzString );
				if ( nCharLen > nLength )
				{
					break;
				}
				Glyph*        pcGlyph = m_pcFont->GetGlyph( utf8_to_unicode( pzString ) );
				pzString += nCharLen;
				nLength  -= nCharLen;
				if ( pcGlyph == NULL )
				{
					printf( "Error: Layer::DrawString() failed to load glyph\n" );
					continue;
				}

				BRect clipRect;

				ENUMBRECTS((*pcReg), clipRect)
				{
					clipRect.OffsetBy(cTopLeft);
					RenderGlyphBlend( m_pcBitmap, pcGlyph, cPos,
								IRect(clipRect), fLayerData->highcolor.GetColor32() );
				}
				fLayerData->penlocation.x += pcGlyph->m_nAdvance;
				cPos.x += pcGlyph->m_nAdvance;
			}
			m_pcBitmap->m_pcDriver->Invalidate(pcReg->Frame().OffsetByCopy(cTopLeft));
			FontServer::Unlock();
		}
	}
	else
	{
		if ( FontServer::Lock() )
		{
			while ( nLength > 0 )
			{
				int nCharLen = utf8_char_length( *pzString );
				if ( nCharLen > nLength ) {
					break;
				}
				Glyph*        pcGlyph = m_pcFont->GetGlyph( utf8_to_unicode( pzString ) );
				pzString += nCharLen;
				nLength  -= nCharLen;
				if ( pcGlyph == NULL )
				{
					printf( "Error: Layer::DrawString() failed to load glyph\n" );
					continue;
				}

				BRect clipRect;

				ENUMBRECTS((*pcReg), clipRect)
				{
					clipRect.OffsetBy(cTopLeft);
					RenderGlyph( m_pcBitmap, pcGlyph, cPos,
								IRect(clipRect), fLayerData->highcolor.GetColor32() );
				}
				cPos.x += pcGlyph->m_nAdvance;
				fLayerData->penlocation.x += pcGlyph->m_nAdvance;
			}
			m_pcBitmap->m_pcDriver->Invalidate(pcReg->Frame().OffsetByCopy(cTopLeft));
			FontServer::Unlock();
		}
	}

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::CopyRect( ServerBitmap* pcBitmap, GRndCopyRect_s* psCmd )
{
	ScrollRect( pcBitmap, psCmd->cSrcRect, psCmd->cDstRect.LeftTop() );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::FillRect( BRect cRect )
{
	if ( NULL == m_pcBitmap )
	{
		return;
	}
	FillRect( cRect, fLayerData->highcolor.GetColor32() );
}


void Layer::FillArc(BRect cRect, float start, float angle, rgb_color sColor)
{
	if ( NULL == m_pcBitmap )
		return;

	BRegion* pcReg = GetRegion();

	if ( NULL == pcReg )
		return;

	BPoint cTopLeft = ConvertToRoot( BPoint(0,0) );
	BRect  cDstRect( cRect.OffsetByCopy(m_cScrollOffset) );
	RGBColor aColor(sColor);

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(cTopLeft))) );
	}
	
	if(fLayerData->clipReg)
		fLayerData->clipReg->MakeEmpty();
	else
		fLayerData->clipReg = new BRegion();

	fLayerData->clipReg->Include(pcReg);
	fLayerData->clipReg->OffsetBy(cTopLeft.x, cTopLeft.y);
	fLayerData->highcolor.SetColor(aColor);
	m_pcBitmap->m_pcDriver->FillArc(cDstRect.OffsetByCopy(cTopLeft), start, angle, fLayerData);
	fLayerData->clipReg->MakeEmpty();

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}


void Layer::StrokeArc(BRect cRect, float start, float angle, rgb_color sColor)
{
	if ( NULL == m_pcBitmap )
		return;

	BRegion* pcReg = GetRegion();

	if ( NULL == pcReg )
		return;

	BPoint cTopLeft = ConvertToRoot( BPoint(0,0) );
	BRect  cDstRect( cRect.OffsetByCopy(m_cScrollOffset) );
	RGBColor aColor(sColor);

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(cTopLeft))) );
	}
	
	if(fLayerData->clipReg)
		fLayerData->clipReg->MakeEmpty();
	else
		fLayerData->clipReg = new BRegion();

	fLayerData->clipReg->Include(pcReg);
	fLayerData->clipReg->OffsetBy(cTopLeft.x, cTopLeft.y);
	fLayerData->highcolor.SetColor(aColor);
	m_pcBitmap->m_pcDriver->StrokeArc(cDstRect.OffsetByCopy(cTopLeft), start, angle, fLayerData);
	fLayerData->clipReg->MakeEmpty();

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}


void Layer::FillEllipse(BRect cRect, rgb_color sColor)
{
	if ( NULL == m_pcBitmap )
		return;

	BRegion* pcReg = GetRegion();

	if ( NULL == pcReg )
		return;

	BPoint cTopLeft = ConvertToRoot( BPoint(0,0) );
	BRect  cDstRect( cRect.OffsetByCopy(m_cScrollOffset) );
	RGBColor aColor(sColor);

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(cTopLeft))) );
	}
	
	if(fLayerData->clipReg)
		fLayerData->clipReg->MakeEmpty();
	else
		fLayerData->clipReg = new BRegion();

	fLayerData->clipReg->Include(pcReg);
	fLayerData->clipReg->OffsetBy(cTopLeft.x, cTopLeft.y);
	fLayerData->highcolor.SetColor(aColor);
	m_pcBitmap->m_pcDriver->FillEllipse(cDstRect.OffsetByCopy(cTopLeft), fLayerData);
	fLayerData->clipReg->MakeEmpty();

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}


void Layer::StrokeEllipse(BRect cRect, rgb_color sColor)
{
	if ( NULL == m_pcBitmap )
	{
		return;
	}

	BRegion* pcReg = GetRegion();

	if ( NULL == pcReg )
	{
		return;
	}

	BPoint cTopLeft = ConvertToRoot( BPoint(0,0) );
	BRect  cDstRect( cRect.OffsetByCopy(m_cScrollOffset) );
	RGBColor aColor(sColor);

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(cTopLeft))) );
	}
	
	if(fLayerData->clipReg)
		fLayerData->clipReg->MakeEmpty();
	else
		fLayerData->clipReg = new BRegion();

	fLayerData->clipReg->Include(pcReg);
	fLayerData->clipReg->OffsetBy(cTopLeft.x, cTopLeft.y);
	fLayerData->highcolor.SetColor(aColor);
	m_pcBitmap->m_pcDriver->StrokeEllipse(cDstRect.OffsetByCopy(cTopLeft), fLayerData);
	fLayerData->clipReg->MakeEmpty();

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}


void Layer::FillRect( BRect cRect, rgb_color sColor )
{
	if ( NULL == m_pcBitmap )
	{
		return;
	}

	BRegion* pcReg = GetRegion();

	if ( NULL == pcReg )
	{
		return;
	}

	BPoint cTopLeft = ConvertToRoot( BPoint(0,0) );
	BRect  cDstRect( cRect.OffsetByCopy(m_cScrollOffset) );
	BRect  aRect;
	RGBColor aColor(sColor);

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(cTopLeft))) );
	}
#if 0
	ENUMBRECTS((*pcReg), aRect)
	{
		aRect = (aRect & cDstRect);
		if ( aRect.IsValid() )
		{
			aRect.OffsetBy(cTopLeft);
			m_pcBitmap->m_pcDriver->FillRect(aRect, aColor);
		}
	}
#endif
	if(fLayerData->clipReg)
		fLayerData->clipReg->MakeEmpty();
	else
		fLayerData->clipReg = new BRegion();

	fLayerData->clipReg->Include(pcReg);
	fLayerData->clipReg->OffsetBy(cTopLeft.x, cTopLeft.y);
	fLayerData->highcolor.SetColor(aColor);
	m_pcBitmap->m_pcDriver->FillRect(cDstRect.OffsetByCopy(cTopLeft), fLayerData);
	fLayerData->clipReg->MakeEmpty();

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::StrokeRect( BRect cRect, rgb_color sColor )
{
	// FIXME: this should use the driver's StrokeRect method if available
	SetHighColor(sColor);
	MovePenTo( cRect.left, cRect.bottom );
	DrawLine( BPoint( cRect.left, cRect.top ) );
	DrawLine( BPoint( cRect.right, cRect.top ) );
	DrawLine( BPoint( cRect.right, cRect.bottom ) );
	DrawLine( BPoint( cRect.left, cRect.bottom ) );
}

#if 0
//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
//        To be removed when ScrollRect() is repaired.
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::BitBlit( ServerBitmap* pcBitmap, BRect cSrcRect, BPoint cDstPos )
{
	ConvertToRoot( &cSrcRect );
	ConvertToRoot( &cDstPos );

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(ConvertToRoot(BPoint())))) );
	}
	pcBitmap->m_pcDriver->BltBitmap( pcBitmap, pcBitmap, static_cast<IRect>(cSrcRect.OffsetByCopy(m_cScrollOffset)),
									IPoint(cDstPos + m_cScrollOffset), fLayerData->draw_mode );
	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}
#endif

void Layer::ScrollRect( ServerBitmap* pcBitmap, BRect cSrcRect, BPoint cDstPos )
{
	if ( _visible == NULL )
	{
		return;
	}

	BPoint	cDelta = cDstPos - cSrcRect.LeftTop();
	BRect	cDstRect(cSrcRect);

	cDstRect.OffsetBy(cDelta);

	BRect	srcRect;
	BRect	dstRect;
	BRegion cBltList;
	BRegion	cDamage(*_visible);
	
	cDamage.OffsetBy(-cDstRect.left, -cDstRect.top);

	ENUMBRECTS((*_visible), srcRect)
	{
		// Clip to source rectangle
		BRect cSRect(cSrcRect & srcRect);

		if (cSRect.IsValid() == false)
			continue;

		// Transform into destination space
		cSRect.OffsetBy(cDelta);

		ENUMBRECTS((*_visible), dstRect)
		{
			BRect cDRect = cSRect & dstRect;

			if ( cDRect.IsValid() == false )
				continue;

			cDamage.Exclude(cDRect);
			clipping_rect pcClip = to_clipping_rect(cDRect);
			pcClip.move_x = cDelta.x;
			pcClip.move_y = cDelta.y;

			cBltList.Include(pcClip);
		}
	}

	int nCount = cBltList.CountRects();

	if ( nCount == 0 )
	{
		Invalidate( cDstRect );
		UpdateIfNeeded( true );
		return;
	}

	IPoint cTopLeft( ConvertToRoot( BPoint() ) );

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect((Bounds().OffsetByCopy(ConvertToRoot( BPoint() )))) );
	}

#if 0
	clipping_rect cliprect;
	
	ENUMCLIPRECTS(cBltList, cliprect)
	{
		offset_rect(cliprect, cTopLeft.x, cTopLeft.y); // Convert into screen space
		IRect icliprect(to_BRect(cliprect));
		icliprect -= IPoint(cliprect.move_x, cliprect.move_y);
		m_pcBitmap->m_pcDriver->BltBitmap(m_pcBitmap,
										  m_pcBitmap,
										  icliprect,
										  IPoint(cliprect.left, cliprect.top),
										  B_OP_COPY);
	}
#else
	// TODO: Use DDriver->Blit()
#endif
	
	if ( _invalid != NULL)
	{
		BRect dmgRect;
		BRegion cReg(*_invalid);
		cReg.OffsetBy(cSrcRect.left, cSrcRect.top);

		ENUMBRECTS(cReg, dmgRect)
		{
			dmgRect.OffsetBy(cDelta);
			_invalid->Include( (dmgRect & cDstRect) );
			if ( m_pcActiveDamageReg != NULL )
			{
				m_pcActiveDamageReg->Exclude( (dmgRect & cDstRect) );
			}
		}
	}

	if ( m_pcActiveDamageReg != NULL)
	{
		BRect dmgRect;
		BRegion cReg(*m_pcActiveDamageReg);
		cReg.OffsetBy(-cSrcRect.left, -cSrcRect.top);
		if ( cReg.CountRects() > 0 )
		{
			if ( _invalid == NULL )
			{
				_invalid = new BRegion();
			}
				ENUMBRECTS(cReg, dmgRect)
				{
					dmgRect.OffsetBy(cDelta);
					m_pcActiveDamageReg->Exclude( (dmgRect & cDstRect) );
					_invalid->Include( (dmgRect & cDstRect) );
				}
		}
	}

	ENUMBRECTS(cDamage, dstRect)
	{
		Invalidate(dstRect);
	}

	UpdateIfNeeded( true );
	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::DrawBitMap( ServerBitmap* pcDstBitmap, ServerBitmap* pcBitMap, BRect cSrcRect, BPoint cDstPos )
{
	BRegion*        pcReg = GetRegion();

	if ( NULL == pcReg )
		return;

	IPoint cTopLeft( ConvertToRoot( BPoint(0,0) ) );
	BRect  clipRect;

	IPoint cIDstPos( cDstPos +  m_cScrollOffset );
	BRect  cDstRect = cSrcRect.OffsetToCopy(cDstPos);
	IPoint cSrcPos( cSrcRect.LeftTop() );

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Hide( IRect(Bounds()) + cTopLeft );
	}
#if 0
	ENUMBRECTS((*pcReg), clipRect)
	{
		IRect cRect(cDstRect & clipRect);

		if ( cRect.IsValid() )
		{
			IPoint cDst = cRect.LeftTop() + cTopLeft;
			IRect  cSrc = cRect - cIDstPos + cSrcPos;

			pcDstBitmap->m_pcDriver->BltBitmap( pcDstBitmap, pcBitMap, cSrc, cDst, fLayerData->draw_mode );
		}
	}
#endif
	if(fLayerData->clipReg)
		fLayerData->clipReg->MakeEmpty();
	else
		fLayerData->clipReg = new BRegion();

// TODO: double check the following in regards to cSrcRect and cDstRect
	fLayerData->clipReg->Include(pcReg);
	fLayerData->clipReg->OffsetBy(cTopLeft.x, cTopLeft.y);
	pcDstBitmap->m_pcDriver->DrawBitmap(pcBitMap, cSrcRect, cDstRect, fLayerData);
	fLayerData->clipReg->MakeEmpty();

	if ( fServerWin == NULL || fServerWin->IsOffScreen() == false )
	{
		SrvSprite::Unhide();
	}
}
