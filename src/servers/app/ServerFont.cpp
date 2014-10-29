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
//	File Name:		ServerFont.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BFont class
//  
//------------------------------------------------------------------------------

#include "AppServer.h"
#include "FontFamily.h"
#include "ServerFont.h"
#include "FontServer.h"

#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <InterfaceDefs.h>
#include <Locker.h>

#include <macros.h>

#define FLOOR(x)  ((x) & -64)
#define CEIL(x)   (((x)+63) & -64)

BLocker g_cFontLock( "font_lock" );

/*! 
	\brief Constructor
	\param style Style object to which the ServerFont belongs
	\param size Character size in points
	\param rotation Rotation in degrees
	\param shear Shear (slant) in degrees. 45 <= shear <= 135
	\param flags Style flags as defined in <Font.h>
	\param spacing String spacing flag as defined in <Font.h>
*/
ServerFont::ServerFont(FontStyle *style, float size, float rotation, float shear)
{
	m_bFixedWidth = style->IsFixedWidth();
	g_cFontLock.Lock();

	fstyle = style;
	m_nGlyphCount = style->GlyphCount();

	if ( m_nGlyphCount > 0 )
	{
		m_ppcGlyphTable = new Glyph*[ m_nGlyphCount ];
		memset( m_ppcGlyphTable, 0, m_nGlyphCount * sizeof( Glyph* ) );
	}
	else
	{
		m_ppcGlyphTable = NULL;
	}
	fsize        = size;
	frotation    = rotation;
	fshear       = shear;

	FT_Face psFace = fstyle->GetFace();
	if ( psFace->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		FT_Set_Char_Size( psFace, fsize, fsize, 96, 96 );
	}
	else
	{
		FT_Set_Pixel_Sizes( psFace, 0, (fsize*96/72) / 64 );
	}
	FT_Size psSize = psFace->size;


	m_nNomWidth  = psSize->metrics.x_ppem;
	m_nNomHeight = psSize->metrics.y_ppem;

	if (psSize->metrics.descender > 0)
	{
		m_nDescender = -((psSize->metrics.descender + 63) / 64);
	}
	else
	{
		m_nDescender = ((psSize->metrics.descender - 63) / 64);
	}

	m_nAscender  = (psSize->metrics.ascender + 63) / 64;
	m_nLineGap   = ((psSize->metrics.height + 63) / 64) - (m_nAscender - m_nDescender);
	m_nAdvance   = (psSize->metrics.max_advance + 63) / 64;

//    printf( "Size1(%d): %ld, %ld, %ld (%ld, %ld, %ld)\n", nPointSize, psSize->metrics.ascender, psSize->metrics.descender, psSize->metrics.height,
//            psSize->metrics.ascender / 64, psSize->metrics.descender / 64, psSize->metrics.height / 64 );

	// Register our self with the font

	fstyle->AddInstance( this );

	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t ServerFont::SetProperties( float size, float shear, float rot )
{
	g_cFontLock.Lock();

	fstyle->RemoveInstance( this );

	fsize     = size;
	frotation = rot;
	fshear    = shear;

	FT_Face psFace = fstyle->GetFace();
	if ( psFace->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		FT_Set_Char_Size( psFace, fsize, fsize, 96, 96 );
	}
	else
	{
		FT_Set_Pixel_Sizes( psFace, 0, (fsize*96/72) / 64 );
	}

	FT_Size psSize = psFace->size;

	m_nNomWidth    = psSize->metrics.x_ppem;
	m_nNomHeight   = psSize->metrics.y_ppem;

	if (psSize->metrics.descender > 0)
	{
		m_nDescender = -((psSize->metrics.descender + 63) / 64);
	}
	else
	{
		m_nDescender = ((psSize->metrics.descender - 63) / 64);
	}

	m_nAscender  = (psSize->metrics.ascender + 63) / 64;
//    m_nLineGap   = (psSize->metrics.height - (psSize->metrics.ascender - psSize->metrics.descender) + 63) / 64;

	m_nLineGap   = ((psSize->metrics.height + 63) / 64) - (m_nAscender - m_nDescender);

//    printf( "Size2(%d): %ld, %ld, %ld (%ld, %ld, %ld)\n", nPointSize, psSize->metrics.ascender, psSize->metrics.descender, psSize->metrics.height,
//            psSize->metrics.ascender / 64, psSize->metrics.descender / 64, psSize->metrics.height / 64 );

	m_nAdvance   = (psSize->metrics.max_advance + 63) / 64;

	for ( int i = 0 ; i < m_nGlyphCount ; ++i )
	{
		if ( m_ppcGlyphTable[i] != NULL )
		{
			delete[] reinterpret_cast<char*>(m_ppcGlyphTable[i]);
			m_ppcGlyphTable[i] = NULL;
		}
	}
	fstyle->AddInstance( this );

	g_cFontLock.Unlock();

	return( 0 );
}

/*! 
	\brief Removes itself as a dependency of its owning style.
*/
ServerFont::~ServerFont(void)
{
	g_cFontLock.Lock();
	fstyle->RemoveInstance( this );

	for ( uint32 i = 0 ; i < m_nGlyphCount ; ++i )
	{
		delete[] reinterpret_cast<char*>(m_ppcGlyphTable[i]);
	}
	delete[] m_ppcGlyphTable;

	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Glyph* ServerFont::GetGlyph( uint32 nIndex )
{
	if ( nIndex < 0 || nIndex >= m_nGlyphCount )
	{
		return( NULL );
	}

	if ( m_ppcGlyphTable[ nIndex ] != NULL )
	{
		return( m_ppcGlyphTable[ nIndex ] );
	}

	FT_Error nError;

	FT_Size psSize = fstyle->GetFace()->size;

	if ( psSize->face->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		FT_Set_Char_Size( psSize->face, fsize, fsize, 96, 96 );
	}
	else
	{
		FT_Set_Pixel_Sizes( psSize->face, 0, (fsize*96/72) / 64 );
	}

	nError = FT_Load_Glyph( psSize->face, nIndex, FT_LOAD_DEFAULT );

	if ( nError != 0 )
	{
		printf( "Unable to load glyph %lu (%f) -> %02x\n", nIndex, (fsize*96/72) / 64, nError );
		if ( nIndex != 0 ) {
			return( GetGlyph(0) );
		} else {
			return( NULL );
		}
	}
	FT_GlyphSlot  glyph = psSize->face->glyph;

	int nLeft;
	int nTop;
	if ( psSize->face->face_flags & FT_FACE_FLAG_SCALABLE )
	{
		nLeft   = FLOOR( glyph->metrics.horiBearingX ) / 64;
		nTop    = -CEIL( glyph->metrics.horiBearingY ) / 64;
	}
	else
	{
		nLeft = glyph->bitmap_left;
		nTop  = -glyph->bitmap_top;
	}

	if ( fstyle->IsScalable() )
	{
		nError = FT_Render_Glyph( glyph, ft_render_mode_normal );

		if ( nError != 0 )
		{
			printf( "Failed to render glyph, err = %x\n", nError );
			if ( nIndex != 0 )
			{
				return( GetGlyph(0) );
			}

			return( NULL );
		}
	}

	if ( glyph->bitmap.width < 0 || glyph->bitmap.rows < 0 || glyph->bitmap.pitch < 0)
	{
		printf( "Error: Glyph got invalid size %dx%d (%d)\n", glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap.pitch );
		if ( nIndex != 0 )
		{
			return( GetGlyph(0) );
		}

		return( NULL );
	}
	IRect cBounds( nLeft, nTop, nLeft + glyph->bitmap.width - 1, nTop + glyph->bitmap.rows - 1 );

	int nRasterSize;
	if ( glyph->bitmap.pixel_mode == ft_pixel_mode_grays )
	{
		nRasterSize = glyph->bitmap.pitch * glyph->bitmap.rows;
	}
	else if ( glyph->bitmap.pixel_mode == ft_pixel_mode_mono )
	{
		nRasterSize = glyph->bitmap.width * glyph->bitmap.rows;
	}
	else
	{
		printf( "ServerFont::GetGlyph() unknown pixel mode : %d\n", glyph->bitmap.pixel_mode );
		if ( nIndex != 0 )
		{
			return( GetGlyph(0) );
		}

		return( NULL );
	}
	try {
		m_ppcGlyphTable[ nIndex ] = (Glyph*) new char[sizeof(Glyph) + nRasterSize];
	} catch(...) {
		return( NULL );
	}
	Glyph* pcGlyph = m_ppcGlyphTable[ nIndex ];

	pcGlyph->m_pRaster = (uint8*)(pcGlyph + 1);
	pcGlyph->m_cBounds = cBounds;

	if ( fstyle->IsScalable() )
	{
		if ( IsFixedWidth() )
		{
			pcGlyph->m_nAdvance = m_nAdvance;
		}
		else
		{
			pcGlyph->m_nAdvance = (glyph->metrics.horiAdvance + 32) / 64;
		}
	}
	else
	{
		pcGlyph->m_nAdvance = glyph->bitmap.width;
	}

	if ( glyph->bitmap.pixel_mode == ft_pixel_mode_grays )
	{
		pcGlyph->m_nBytesPerLine = glyph->bitmap.pitch;
		memcpy( pcGlyph->m_pRaster, glyph->bitmap.buffer, nRasterSize );
	}
	else
	{
		pcGlyph->m_nBytesPerLine = glyph->bitmap.width;

		for ( int y = 0 ; y < pcGlyph->m_cBounds.Height() + 1 ; ++y )
		{
			for ( int x = 0 ; x < pcGlyph->m_cBounds.Width() + 1 ; ++x )
			{
				if ( glyph->bitmap.buffer[x/8+y*glyph->bitmap.pitch] & (1<<(7-(x%8))) )
				{
					pcGlyph->m_pRaster[x+y*pcGlyph->m_nBytesPerLine] = 255;
				}
				else
				{
					pcGlyph->m_pRaster[x+y*pcGlyph->m_nBytesPerLine] = 0;
				}
			}
		}
	}
	return( m_ppcGlyphTable[ nIndex ] );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ServerFont::GetStringWidth( const char* pzString, int nLength )
{
	int        nWidth        = 0;

	g_cFontLock.Lock();
	while ( nLength > 0 )
	{
		int nCharLen = utf8_char_length( *pzString );
		if ( nCharLen > nLength )
		{
			break;
		}
		Glyph*        pcGlyph = GetGlyph( FT_Get_Char_Index( fstyle->GetFace(), utf8_to_unicode( pzString ) ) );
		pzString += nCharLen;
		nLength  -= nCharLen;
		if ( pcGlyph == NULL )
		{
			printf( "Error: GetStringWidth() failed to load glyph\n" );
			continue;
		}
		nWidth += pcGlyph->m_nAdvance;
	}
	g_cFontLock.Unlock();
	return( nWidth );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ServerFont::GetStringLength( const char* pzString, int nLength, int nWidth, bool bIncludeLast )
{
	int nStrLen = 0;
	g_cFontLock.Lock();
	while ( nLength > 0 )
	{
		int nCharLen = utf8_char_length( *pzString );
		if ( nCharLen > nLength )
		{
			break;
		}
		Glyph*        pcGlyph = GetGlyph( FT_Get_Char_Index( fstyle->GetFace(), utf8_to_unicode( pzString ) ) );

		if ( pcGlyph == NULL )
		{
			printf( "Error: GetStringLength() failed to load glyph\n" );
			break;
		}
		if ( nWidth < pcGlyph->m_nAdvance )
		{
			if ( bIncludeLast )
			{
				nStrLen  += nCharLen;
			}
			break;
		}
		pzString += nCharLen;
		nLength  -= nCharLen;
		nStrLen  += nCharLen;
		nWidth -= pcGlyph->m_nAdvance;
	}
	g_cFontLock.Unlock();
	return( nStrLen );
}

int ServerFont::CharToUnicode( int nChar )
{
return( fstyle->ConvertToUnicode( nChar ) );
}

/*! 
	\brief Returns the number of strikes in the font
	\return The number of strikes in the font
*/
int32 ServerFont::CountTuned(void)
{
	if(fstyle)
		fstyle->TunedCount();

	return 0;
}

/*! 
	\brief Returns the file format of the font. Currently unimplemented.
	\return B_TRUETYPE_WINDOWS
*/
font_file_format ServerFont::FileFormat(void)
{
	// TODO: implement
	return 	B_TRUETYPE_WINDOWS;
}

/*! 
	\brief Returns a BRect which encloses the entire font
	\return A BRect which encloses the entire font
*/
BRect ServerFont::BoundingBox(void)
{
	return fbounds;
}

/*! 
	\brief Obtains the height values for characters in the font in its current state
	\param fh pointer to a font_height object to receive the values for the font
*/
void ServerFont::Height(font_height *fh)
{
	fh->ascent=fheight.ascent;
	fh->descent=fheight.descent;
	fh->leading=fheight.leading;
}

/*
 @log
	* added ServerFont::operator=(const ServerFont& font).
*/
