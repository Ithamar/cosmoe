/*  libcosmoe.so - the interface to the Cosmoe UI
 *  Portions Copyright (C) 2001-2002 Bill Hayden
 *  Portions Copyright (C) 1999-2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */


#include <assert.h>

#include <Bitmap.h>
#include "stdbitmaps.h"



#define MAX_BM_SIZE (8*8)

struct BitmapDesc
{
	int w;
	int h;
	int y;
	uint8 anRaster[MAX_BM_SIZE];
};

BitmapDesc g_asBitmapDescs[] =
{
	{ 4, 7, 0, { // BMID_ARROW_LEFT
		0xff,0xff,0xff,0x00,
		0xff,0xff,0x00,0x00,
		0xff,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,
		0xff,0x00,0x00,0x00,
		0xff,0xff,0x00,0x00,
		0xff,0xff,0xff,0x00
	}},
	{ 4, 7, 0, { // BMID_ARROW_RIGHT
		0x00,0xff,0xff,0xff,
		0x00,0x00,0xff,0xff,
		0x00,0x00,0x00,0xff,
		0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0xff,
		0x00,0x00,0xff,0xff,
		0x00,0xff,0xff,0xff
	}},
	{ 7, 4, 0, { // BMID_ARROW_UP
		0xff,0xff,0xff,0x00,0xff,0xff,0xff,
		0xff,0xff,0x00,0x00,0x00,0xff,0xff,
		0xff,0x00,0x00,0x00,0x00,0x00,0xff,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00
	}},
	{ 7, 4, 0, { // BMID_ARROW_DOWN
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xff,0x00,0x00,0x00,0x00,0x00,0xff,
		0xff,0xff,0x00,0x00,0x00,0xff,0xff,
		0xff,0xff,0xff,0x00,0xff,0xff,0xff
	}},
	{ 0, 0, 0, { 0x00 }}, // End marker
};



static BBitmap* g_pcBitmap = NULL;


BBitmap* get_std_bitmap( int nBitmap, int nColor, BRect* pcRect )
{
	assert( nBitmap >= 0 && nBitmap < BMID_COUNT );
	assert( nColor >= 0 && nColor < 3 );

	if ( g_pcBitmap == NULL )
	{
		int nWidth  = 0;
		int nHeight = 0;
		for ( int i = 0 ; g_asBitmapDescs[i].w > 0 ; ++i )
		{
			if ( g_asBitmapDescs[i].w > nWidth )
			{
				nWidth = g_asBitmapDescs[i].w;
			}
			g_asBitmapDescs[i].y = nHeight;
			nHeight += g_asBitmapDescs[i].h * 3;
		}

		g_pcBitmap = new BBitmap( BRect(0, 0, nWidth, nHeight), B_COLOR_8_BIT );

		uint8* pRaster = (uint8*)(g_pcBitmap->Bits());
		int nPitch = g_pcBitmap->BytesPerRow();

		for ( int i = 0 ; g_asBitmapDescs[i].w > 0 ; ++i )
		{
			int w = g_asBitmapDescs[i].w;
			int h = g_asBitmapDescs[i].h;
			assert( w * h < MAX_BM_SIZE );

			for ( int y = 0 ; y < h ; ++y )
			{
				for ( int x = 0 ; x < w ; ++x )
				{
					*pRaster++ = g_asBitmapDescs[i].anRaster[y*w+x]; // Black
				}
				pRaster += nPitch - w;
			}

			for ( int y = 0 ; y < h ; ++y )
			{
				for ( int x = 0 ; x < w ; ++x )
				{
					*pRaster++ = (g_asBitmapDescs[i].anRaster[y*w+x] == 0xff) ? 0xff : 14; // Gray
				}
				pRaster += nPitch - w;
			}

			for ( int y = 0 ; y < h ; ++y )
			{
				for ( int x = 0 ; x < w ; ++x )
				{
					*pRaster++ = (g_asBitmapDescs[i].anRaster[y*w+x] == 0xff) ? 0xff : 63; // White
				}
				pRaster += nPitch - w;
			}
		}
	}
//    int nIndex = nBitmap * 3 + nColor;
	int y = g_asBitmapDescs[nBitmap].y + g_asBitmapDescs[nBitmap].h * nColor;
	*pcRect = BRect( 0, y, g_asBitmapDescs[nBitmap].w - 1, y + g_asBitmapDescs[nBitmap].h - 1 );
	return( g_pcBitmap );
}
