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
//	File Name:		ServerFont.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Shadow BFont class
//  
//------------------------------------------------------------------------------
#ifndef SERVERFONT_H_
#define SERVERFONT_H_

#include <SupportDefs.h>

#include <Rect.h>
#include <Font.h>
#include <IRect.h>
#include <InterfaceDefs.h>
#include <util/resource.h>
#include <Locker.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <string>

class        FontFamily;
class        FontStyle;
class        ServerFont;

extern BLocker g_cFontLock;


struct Glyph
{
	int         m_nAdvance;
	IRect       m_cBounds;
	int         m_nBytesPerLine;
	uint8*      m_pRaster;
	ServerFont* m_pcInstance;
};

class ServerFont : public Resource
{
public:
	ServerFont( FontStyle* pcFont, float size, float rot, float shear);

	status_t	SetProperties( const font_properties& sProps );
	status_t	SetProperties( float size, float shear, float rot );
	int			GetIdentifier() const             { return( m_nIdentifier );        }

	font_direction Direction(void) const { return fdirection; }
	uint32 Encoding(void) const { return fencoding; }
	edge_info Edges(void) const { return fedges; }
	uint32 Flags(void) const { return fflags; }
	uint32 Spacing(void) const { return fspacing; }
	float Shear(void) const { return fshear; }
	float Rotation(void) const { return frotation; }
	float Size(void) const { return fsize; }
	uint32 Face(void) const { return fface; }
	uint32 CountGlyphs(void) const             { return( m_nGlyphCount );        }
	int32 CountTuned(void);
	font_file_format FileFormat(void);
	FontStyle *Style(void) const { return fstyle; }

	void SetDirection(const font_direction &dir) { fdirection=dir; }
	void SetEdges(const edge_info &info) { fedges=info; }
	void SetEncoding(uint32 encoding) { fencoding=encoding; }
	void SetFlags(const uint32 &value) { fflags=value; }
	void SetSpacing(const uint32 &value) { fspacing=value; }
	void SetShear(const float &value) { fshear=value; }
	void SetSize(const float &value) { fsize=value; }
	void SetRotation(const float &value) { frotation=value; }
	void SetFace(const uint32 &value) { fface=value; }

	BRect BoundingBox(void);
	void Height(font_height *fh);

	Glyph*		GetGlyph( uint32 nIndex );
	int	CharToUnicode( int nChar );

	void        SetFixedWidth( bool bFixed )      { m_bFixedWidth = bFixed; }
	bool        IsFixedWidth()                    { return( m_bFixedWidth );                }

	int			GetNomWidth() const               { return( m_nNomWidth );        }
	int			GetNomHeight() const              { return( m_nNomHeight );         }
	int			GetAscender() const               { return( m_nAscender );        }
	int			GetDescender() const              { return( m_nDescender );        }
	int			GetLineGap() const                { return( m_nLineGap );                }
	int			GetAdvance() const                { return( m_nAdvance );         }
	int			GetStringWidth( const char* pzString, int nLength );
	int			GetStringLength( const char* pzString, int nLength, int nWidth, bool bIncludeLast );

protected:
				~ServerFont();
	friend class FontStyle;
	friend class FontServer;
	FontStyle *fstyle;

	font_height fheight;
	edge_info fedges;
	float fsize, frotation, fshear;
	BRect fbounds;
	uint32 fflags;
	uint32 fspacing;
	uint32 fface;
	font_direction fdirection;
	uint8 ftruncate;
	uint32 fencoding;

	bool		m_bFixedWidth;
	int			m_nIdentifier;  // Unique identifier, sent to applications

	int			m_nNomWidth;
	int			m_nNomHeight;

	int			m_nAscender;     // Space above baseline in pixels
	int			m_nDescender;    // Space below baseline in pixels
	int			m_nLineGap;      // Space between lines in pixels
	int			m_nAdvance;      // Max advance in pixels (Used for monospaced fonts)

	Glyph**		m_ppcGlyphTable; // Array of pointers to glyps. Each unloaded glyp has a NULL pointer.
	uint32		m_nGlyphCount;
};

#endif
/*
 @log
 	* added '=' operator
*/
