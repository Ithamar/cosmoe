//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Font.cpp
//	Author:			DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	Class to manage font-handling capabilities
//------------------------------------------------------------------------------
#include <Rect.h>
#include <stdio.h>
#include <Font.h>

#include <string.h>
#include <assert.h>
#include <stdexcept>
#include <errno.h>

#include <kernel.h>

#include <Application.h>
#include <Message.h>
#include <Messenger.h>

#include <ServerProtocol.h>


//----------------------------------------------------------------------------------------
//		Globals
//----------------------------------------------------------------------------------------

// The actual objects which the globals point to
//BFont be_plain_bfont;
//BFont be_bold_bfont;
//BFont be_fixed_bfont;

//const BFont *be_plain_font=&be_plain_bfont;
//const BFont *be_bold_font=&be_bold_bfont;
//const BFont *be_fixed_font=&be_fixed_bfont;

/*!
	\brief Private function used by Be. Exists only for compatibility. Does nothing.
*/
void _font_control_(BFont *font, int32 cmd, void *data)
{
}

/*!
	\brief Returns the number of installed font families
	\return The number of installed font families
*/
int32 count_font_families(void)
{
	// TODO: Implement
	return 0;
}

/*!
	\brief Returns the number of styles available for a font family
	\return The number of styles available for a font family
*/
int32 count_font_styles(font_family name)
{
	// TODO: Implement
	return 0;
}

/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param flags iF non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font family
*/
status_t get_font_family(int32 index, font_family *name, uint32 *flags)
{
	// Fix over R5, which does not check for NULL font family names - it just crashes
	if(!name)
		return B_ERROR;
	
	// TODO: Implement
}

/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param flags iF non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font style
*/
status_t get_font_style(font_family family, int32 index, font_style *name, uint32 *flags)
{
	// Fix over R5, which does not check for NULL font style names - it just crashes
	if(!name)
		return B_ERROR;

	// TODO: Implement
}

/*!
	\brief Retrieves the family name at the specified index
	\param index Unique font identifier code.
	\param name font_family string to receive the name of the family
	\param face recipient of font face value, such as B_REGULAR_FACE
	\param flags iF non-NULL, the values of the flags IS_FIXED and B_HAS_TUNED_FONT are returned
	\return B_ERROR if the index does not correspond to a font style
	
	The face value returned by this function is not very reliable. At the same time, the value
	returned should be fairly reliable, returning the proper flag for 90%-99% of font names.
*/
status_t get_font_style(font_family family, int32 index, font_style *name,
			uint16 *face, uint32 *flags)
{
	// Fix over R5, which does not check for NULL font style names - it just crashes
	if(!name || !face)
		return B_ERROR;

	// TODO: Implement
}

/*!
	\brief Updates the font family list
	\param check_only If true, the function only checks to see if the font list has changed
	\return true if the font list has changed, false if not.

	Because of the differences in the R5 and OpenBeOS font subsystems, this function operates 
	slightly differently, resulting in more efficient operation. A global font list for all 
	applications is maintained, so calling this function will still be quite expensive, 
	but it should be unnecessary in most applications.
*/
bool update_font_families(bool check_only)
{
	// TODO: Implement
}

status_t get_font_cache_info(uint32 id, void *set)
{
	// TODO: Implement

	// Note that the only reliable data from this function will probably be the cache size
	// Depending on how the font cache is implemented, this function and the corresponding
	// set function will either see major revision or completely disappear in R2.
}

status_t set_font_cache_info(uint32 id, void *set)
{
	// TODO: Implement

	// Note that this function will likely only set the cache size in our implementation
	// because of (a) the lack of knowledge on R5's font system and (b) the fact that it
	// is a completely different font engine.
}

void BFont::_CommonInit()
{
	BMessage cReq( AR_CREATE_FONT );
	BMessage cReply;

	assert( be_app != NULL );

	m_nRefCount   = 1;
	fSize       = 10.0f;
	fShear      = 0.0f;
	fRotation   = 0.0f;
	fSpacing    = B_CHAR_SPACING;
	fEncoding   = 0;
	fFlags      = 0;
	m_hFontHandle = -1;
	m_hReplyPort  = create_port( 15, "font_reply" );

	port_id hPort = be_app->m_hSrvAppPort;
	BMessenger( hPort ).SendMessage( &cReq, &cReply );
	cReply.FindInt32( "handle", &m_hFontHandle );
}

//----------------------------------------------------------------------------------------
//		BFont Class Definition
//----------------------------------------------------------------------------------------

BFont::BFont(void) 
	//initialise for be_plain_font (avoid circular definition)
 : 	fFamilyID(0), fStyleID(0), fSize(10.0), fShear(0.0), fRotation(0.0),
	fSpacing(0), fEncoding(0), fFace(0), fFlags(0)
{
	_CommonInit();
}

BFont::BFont(const BFont &font)
{
	_CommonInit();

	// Make sure SetProperties() really takes action
	fSize     = -1.0f;
	fRotation = -2.0f;
	fShear    = -3.0f;

	SetFamilyAndStyle( font.m_cFamily.c_str(), font.m_cStyle.c_str() );
	SetProperties( font.fSize, font.fShear, font.fRotation );
}

#if 0
BFont::BFont(const std::string& cConfigName )
{
	_CommonInit();
	font_properties sProps;
	if ( GetDefaultFont( cConfigName, &sProps ) != 0 )
	{
		throw( std::runtime_error( "Configuration not found" ) );
	}
	if ( SetProperties( sProps ) != 0 )
	{
		throw( std::runtime_error( "Failed to set properties" ) );
	}
}

BFont::BFont( const font_properties& sProps )
{
	_CommonInit();
	if ( SetProperties( sProps ) != 0 )
	{
		throw( std::runtime_error( "Failed to set properties" ) );
	}
}
#endif

BFont::~BFont()
{
	assert( be_app != NULL );
	assert( m_nRefCount == 0 );

	BMessage cReq( AR_DELETE_FONT );
	cReq.AddInt32( "handle", m_hFontHandle );

	port_id hPort = be_app->m_hSrvAppPort;
	BMessenger( hPort ).SendMessage( &cReq );
	delete_port( m_hReplyPort );
}


void BFont::AddRef()
{
	atomic_add( &m_nRefCount, 1 );
}


void BFont::Release()
{
	if ( atomic_add( &m_nRefCount, -1 ) == 1 )
	{
		delete this;
	}
}


status_t BFont::SetProperties( const std::string& cConfigName )
{
	status_t nError;
	font_properties sProps;

	nError = GetDefaultFont( cConfigName, &sProps );
	if ( nError != 0 )
	{
		return( nError );
	}
	return( SetProperties( sProps ) );
}


status_t BFont::SetProperties( const font_properties& sProps )
{
	status_t nError;
	nError = SetFamilyAndStyle( sProps.m_cFamily.c_str(), sProps.m_cStyle.c_str() );
	if ( nError < 0 )
	{
		return( nError );
	}
	return( SetProperties( sProps.m_vSize, sProps.m_vShear, sProps.m_vRotation ) );
}

BFont::BFont(const BFont *font)
{
	if(font)
	{
		fFamilyID=font->fFamilyID;
		fStyleID=font->fStyleID;
		fSize=font->fSize;
		fShear=font->fShear;
		fRotation=font->fRotation;
		fSpacing=font->fSpacing;
		fEncoding=font->fEncoding;
		fFace=font->fFace;
		fHeight=font->fHeight;
	}
}

/*!
	\brief Sets the font's family and style all at once
	\param family Font family to set
	\param style Font style to set
	\return B_ERROR if family or style do not exist or if style does not belong to family.
*/
status_t BFont::SetFamilyAndStyle(const font_family family, const font_style style)
{
	BMessage cReq( AR_SET_FONT_FAMILY_AND_STYLE );
	BMessage cReply;

	cReq.AddInt32( "handle", m_hFontHandle );
	cReq.AddString( "family", family );
	cReq.AddString( "style", style );

	port_id hPort = be_app->m_hSrvAppPort;
	BMessenger( hPort ).SendMessage( &cReq, &cReply );

	int32 nError = -EINVAL;

	cReply.FindInt32( "error", &nError );

	if ( nError >= 0 )
	{
		int32 nAscender;
		int32 nDescender;
		int32 nLineGap;

		cReply.FindInt32( "ascender",  &nAscender );
		cReply.FindInt32( "descender", &nDescender );
		cReply.FindInt32( "leading",  &nLineGap );

		fHeight.ascent  = nAscender;
		fHeight.descent = -nDescender;
		fHeight.leading = nLineGap;

		m_cFamily = family;
		m_cStyle  = style;

		return B_OK;
	}

	errno = -nError;
	return( -1 );
}

/*!
	\brief Sets the font's family and style all at once
	\param code Unique font identifier obtained from the server.
*/
void BFont::SetFamilyAndStyle(uint32 code)
{
	fStyleID=code & 0xFFFF;
	fFamilyID=(code & 0xFFFF0000) >> 16;
}

void BFont::SetSize(float size)
{
	SetProperties( size, fShear, fRotation );
}

void BFont::SetShear(float shear)
{
	SetProperties( fSize, shear, fRotation );
}

void BFont::SetRotation(float rotation)
{
	SetProperties( fSize, fShear, rotation );
}

void BFont::SetSpacing(uint8 spacing)
{
	fSpacing=spacing;
}

void BFont::SetEncoding(uint8 encoding)
{
	fEncoding=encoding;
}

void BFont::SetFace(uint16 face)
{
	fFace=face;
}

void BFont::SetFlags(uint32 flags)
{
	fFlags=flags;
}

status_t BFont::SetProperties( float vSize, float vShear, float vRotation )
{
	if ( vSize != fSize || vShear != fShear || vRotation != vRotation )
	{
		BMessage cReq( AR_SET_FONT_PROPERTIES );
		BMessage cReply;

		cReq.AddInt32( "handle", m_hFontHandle );
		cReq.AddFloat( "size", vSize );
		cReq.AddFloat( "rotation", vRotation );
		cReq.AddFloat( "shear", vShear );

		port_id hPort = be_app->m_hSrvAppPort;
		BMessenger( hPort ).SendMessage( &cReq, &cReply );

		int32 nError = -EINVAL;

		cReply.FindInt32( "error", &nError );

		if ( nError >= 0 )
		{
			int32 nAscender;
			int32 nDescender;
			int32 nLineGap;

			cReply.FindInt32( "ascender",  &nAscender );
			cReply.FindInt32( "descender", &nDescender );
			cReply.FindInt32( "leading",  &nLineGap );

			fHeight.ascent  = nAscender;
			fHeight.descent = -nDescender;
			fHeight.leading = nLineGap;

			fSize        = vSize;
			fRotation    = vRotation;
			fShear       = vShear;

			return( 0 );
		}
		else
		{
			errno = -nError;
			return( -1 );
		}
	}
	return( 0 );
}

void BFont::GetFamilyAndStyle(font_family *family, font_style *style) const
{
	// TODO: implement

	// Query server for the names of this stuff given the family and style IDs kept internally
}

uint32 BFont::FamilyAndStyle(void) const
{
	uint32 token;
	token=(fFamilyID << 16) | fStyleID;
	return 0L;
}

float BFont::Size(void) const
{
	return fSize;
}

float BFont::Shear(void) const
{
	return fShear;
}


float BFont::Rotation(void) const
{
	return fRotation;
}


uint8 BFont::Spacing(void) const
{
	return fSpacing;
}

uint8 BFont::Encoding(void) const
{
	return fEncoding;
}

uint16 BFont::Face(void) const
{
	return fFace;
}

uint32 BFont::Flags(void) const
{
	return fFlags;
}


font_direction BFont::Direction(void) const
{
	// TODO: Query the server for the value

	return B_FONT_LEFT_TO_RIGHT;
}

bool BFont::IsFixed(void) const
{
	// TODO: query server for whether this bad boy is fixed-width
	
	return false;
}

/*!
	\brief Returns true if the font is fixed-width and contains both full and half-width characters
	
	This was left unimplemented as of R5. It was a way to work with both Kanji and Roman 
	characters in the same fixed-width font.
*/
bool BFont::IsFullAndHalfFixed(void) const
{
	return false;
}

BRect BFont::BoundingBox(void) const
{
	// TODO: query server for bounding box
	return BRect(0,0,0,0);
}

unicode_block BFont::Blocks(void) const
{
	// TODO: Add Block support
	return unicode_block();
}

font_file_format BFont::FileFormat(void) const
{
	// TODO: this will not work until I extend FreeType to handle this kind of call
	return B_TRUETYPE_WINDOWS;
}

int32 BFont::CountTuned(void) const
{
	// TODO: query server for appropriate data
	return 0;
}

void BFont::GetTunedInfo(int32 index, tuned_font_info *info) const
{
	// TODO: implement
}

void BFont::TruncateString(BString *in_out,uint32 mode,float width) const
{
	// TODO: implement
}

void BFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
		uint32 mode, float width, BString resultArray[]) const
{
	// TODO: implement
}

void BFont::GetTruncatedStrings(const char *stringArray[], int32 numStrings, 
		uint32 mode, float width, char *resultArray[]) const
{
	// TODO: implement
}

float BFont::StringWidth(const char *string) const
{
	return StringWidth(string, strlen(string));
}


float BFont::StringWidth(const char *string, int32 length) const
{
	const char*  apzStrPtr[] = {string};
	float        vWidth;

	GetStringWidths(apzStrPtr, &length, 1, &vWidth);

	return vWidth;
}


void BFont::GetStringWidths(const char *stringArray[], const int32 lengthArray[], 
		int32 numStrings, float widthArray[]) const
{
	int        i;
	// The first string size, and one byte of the first string is already included
	int        nBufSize = sizeof( AR_GetStringWidths_s ) - sizeof( int ) - 1;        

	for ( i = 0 ; i < numStrings ; ++i )
	{
		nBufSize += lengthArray[ i ] + sizeof(int);
	}

	AR_GetStringWidths_s* psReq;

	char* pBuffer = new char[ nBufSize ];
	psReq = (AR_GetStringWidths_s*) pBuffer;

	psReq->hReply        = m_hReplyPort;
	psReq->hFontToken    = m_hFontHandle;
	psReq->nStringCount  = numStrings;

	int* pnLen = &psReq->sFirstHeader.nLength;

	for ( i = 0 ; i < numStrings ; ++i )
	{
		int nLen = lengthArray[ i ];
		*pnLen = nLen;
		pnLen++;

		memcpy( pnLen, stringArray[i], nLen );
		pnLen = (int*) (((uint8*)pnLen) + nLen);
	}
	
	if ( write_port( be_app->m_hSrvAppPort, AR_GET_STRING_WIDTHS, psReq, nBufSize ) == 0 ) {
		AR_GetStringWidthsReply_s*        psReply = (AR_GetStringWidthsReply_s*) pBuffer;
		int32 msgCode;

		if ( read_port( m_hReplyPort, &msgCode, psReply, nBufSize ) >= 0 )
		{
			if ( 0 == psReply->nError )
			{
				for ( i = 0 ; i < numStrings ; ++i )
				{
					widthArray[ i ] = psReply->anLengths[ i ];
				}
			}
		}
		else
		{
			printf("Error: in BFont::GetStringWidths() read_port returned %ld (%p)\n",
					m_hReplyPort, this );
		}
	}
	else
	{
		printf( "Error: BFont::GetStringWidths() failed to send AR_GET_STRING_WIDTHS to server\n" );
	}
	delete[] pBuffer;
}


int BFont::GetStringLength( const char* pzString, float vWidth, bool bIncludeLast ) const
{
	return( GetStringLength( pzString, strlen( pzString ), vWidth, bIncludeLast ) );
}


int BFont::GetStringLength( const char* pzString, int nLength, float vWidth, bool bIncludeLast ) const
{
	const char*        apzStrPtr[] = { pzString };
	int                nMaxLength;

	GetStringLengths( apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast );

	return( nMaxLength );
}


int BFont::GetStringLength( const std::string& cString, float vWidth, bool bIncludeLast ) const
{
	const char*        apzStrPtr[] = { cString.c_str() };
	int                nMaxLength;
	int                nLength = cString.size();

	GetStringLengths( apzStrPtr, &nLength, 1, vWidth, &nMaxLength, bIncludeLast );

	return( nMaxLength );
}


void BFont::GetStringLengths( const char** apzStringArray, const int* anLengthArray,
                               int nStringCount, float vWidth, int* anMaxLengthArray, bool bIncludeLast ) const
{
	int i;
	// The first string size, and one byte of the first string is already included
	int nBufSize = sizeof( AR_GetStringLengths_s ) - sizeof( int ) - 1;        

	for ( i = 0 ; i < nStringCount ; ++i )
	{
		nBufSize += anLengthArray[ i ] + sizeof( int );
	}

	AR_GetStringLengths_s* psReq;

	char* pBuffer = new char[ nBufSize ];

	psReq = (AR_GetStringLengths_s*) pBuffer;

	psReq->hReply       = m_hReplyPort;
	psReq->hFontToken   = m_hFontHandle;
	psReq->nStringCount = nStringCount;
	psReq->nWidth       = vWidth;
	psReq->bIncludeLast = bIncludeLast;

	int* pnLen = &psReq->sFirstHeader.nLength;

	for ( i = 0 ; i < nStringCount ; ++i )
	{
		int nLen = anLengthArray[ i ];

		*pnLen = nLen;
		pnLen++;

		memcpy( pnLen, apzStringArray[i], nLen );
		pnLen = (int*) (((uint8*)pnLen) + nLen);
	}

	if ( write_port( be_app->m_hSrvAppPort, AR_GET_STRING_LENGTHS, psReq, nBufSize ) == 0 )
	{
		AR_GetStringLengthsReply_s* psReply = (AR_GetStringLengthsReply_s*) pBuffer;
		int32 msgCode;

		if ( read_port( m_hReplyPort, &msgCode, psReply, nBufSize ) >= 0 )
		{
			if ( 0 == psReply->nError )
			{
				for ( i = 0 ; i < nStringCount ; ++i )
				{
					anMaxLengthArray[ i ] = psReply->anLengths[ i ];
				}
			}
		}
		else
		{
			printf( "Error: BFont::GetStringLengths() failed to get reply\n" );
		}
	}
	else
	{
		printf( "Error: BFont::GetStringLengths() failed to send AR_GET_STRING_LENGTHS request to server\n" );
	}
	delete[] pBuffer;
}

void BFont::GetEscapements(const char charArray[], int32 numChars, float escapementArray[]) const
{
	// TODO: implement
}

void BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		float escapementArray[]) const
{
	// TODO: implement
}

void BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		BPoint escapementArray[]) const
{
	// TODO: implement
}

void BFont::GetEscapements(const char charArray[], int32 numChars, escapement_delta *delta, 
		BPoint escapementArray[], BPoint offsetArray[]) const
{
	// TODO: implement
}

void BFont::GetEdges(const char charArray[], int32 numBytes, edge_info edgeArray[]) const
{
	// TODO: implement
}

void BFont::GetHeight(font_height *height) const
{
	if(height)
	{
		*height=fHeight;
	}
}

void BFont::GetBoundingBoxesAsGlyphs(const char charArray[], int32 numChars, font_metric_mode mode,
		BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void BFont::GetBoundingBoxesAsString(const char charArray[], int32 numChars, font_metric_mode mode,
		escapement_delta *delta, BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void BFont::GetBoundingBoxesForStrings(const char *stringArray[], int32 numStrings,
		font_metric_mode mode, escapement_delta deltas[], BRect boundingBoxArray[]) const
{
	// TODO: implement
}

void BFont::GetGlyphShapes(const char charArray[], int32 numChars, BShape *glyphShapeArray[]) const
{
	// TODO: implement
}
   
void BFont::GetHasGlyphs(const char charArray[], int32 numChars, bool hasArray[]) const
{
	// TODO: implement
}

status_t BFont::GetDefaultFont( const std::string& cName, font_properties* psProps )
{
	return( be_app->GetDefaultFont( cName, psProps ) );
}


BFont &BFont::operator=(const BFont &font)
{
	SetFamilyAndStyle( font.m_cFamily.c_str(), font.m_cStyle.c_str() );
	SetProperties( font.fSize, font.fShear, font.fRotation );
	return *this;
}


bool BFont::operator==(const BFont &font) const
{
	return( m_cFamily == font.m_cFamily &&
			m_cStyle == font.m_cStyle &&
			fSize == font.fSize &&
			fShear == font.fShear &&
			fRotation == font.fRotation &&
			fSpacing == font.fSpacing &&
			fEncoding == font.fEncoding &&
			fHeight.ascent!=font.fHeight.ascent ||
			fHeight.descent!=font.fHeight.descent ||
			fHeight.leading!=font.fHeight.leading ||
			fFlags == font.fFlags );        
}


bool BFont::operator!=(const BFont &font) const
{
	return( m_cFamily != font.m_cFamily ||
			m_cStyle != font.m_cStyle ||
			fSize!=font.fSize ||
			fShear!=font.fShear ||
			fRotation!=font.fRotation ||
			fSpacing!=font.fSpacing ||
			fEncoding!=font.fEncoding ||
			fHeight.ascent!=font.fHeight.ascent ||
			fHeight.descent!=font.fHeight.descent ||
			fHeight.leading!=font.fHeight.leading ||
			fFlags != font.fFlags );        
}

void BFont::PrintToStream(void) const
{
	printf("FAMILY STYLE %f %f %f %f %f %f\n", fSize, fShear, fRotation, fHeight.ascent,
		fHeight.descent, fHeight.leading);
}

