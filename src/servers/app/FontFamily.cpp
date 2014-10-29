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
//	File Name:		FontFamily.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	classes to represent font styles and families
//  
//------------------------------------------------------------------------------
#include "FontFamily.h"
#include "ServerFont.h"
#include "FontServer.h"
#include <Debug.h>
#include <macros.h>
#include FT_CACHE_H

#include <assert.h>

FTC_Manager ftmanager;

/*!
	\brief Constructor
	\param filepath path to a font file
	\param face FreeType handle for the font file after it is loaded - for its info only
*/
FontStyle::FontStyle(const char *filepath, FontFamily* pcFamily, FT_Face face)
{
	name=new BString(face->style_name);
	m_psFace   = face;
	family=pcFamily;
	instances=new BList(0);
	has_bitmaps=(face->num_fixed_sizes>0)?true:false;
	is_fixedwidth=(face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)?true:false;
	is_scalable=(face->face_flags & FT_FACE_FLAG_SCALABLE)?true:false;
	has_kerning=(face->face_flags & FT_FACE_FLAG_KERNING)?true:false;
	glyphcount=face->num_glyphs;
	charmapcount=face->num_charmaps;
	tunedcount=face->num_fixed_sizes;
	path=new BString(filepath);
	fbounds.Set(0,0,0,0);

	m_bDeleted          = false;

	for ( int i = 0 ; i < face->num_fixed_sizes ; ++i )
	{
		m_cBitmapSizes.push_back( float(face->available_sizes[i].height)*72.0f/96.0f );
	}
	pcFamily->AddStyle(filepath, this);
}

/*!
	\brief Destructor
	
	Frees all data allocated on the heap. All child ServerFonts are marked as having
	a NULL style so that each ServerFont knows that it is running on borrowed time. 
	This is done because a FontStyle should be deleted only when it no longer has any
	dependencies.
*/
FontStyle::~FontStyle(void)
{
	delete name;
	delete path;

	family->RemoveStyle(Name());
	instances->MakeEmpty();
	delete instances;
}

/*!
	\brief Returns the name of the style as a string
	\return The style's name
*/
const char *FontStyle::Name(void)
{
	return name->String();
}

/*!
	\brief Returns a handle to the style in question, straight from FreeType's cache
	\return FreeType face handle or NULL if there was an internal error
*/
FT_Face FontStyle::GetFace(void)
{
	return m_psFace;
}

/*!
	\brief Returns the path to the style's font file 
	\return The style's font file path
*/
const char *FontStyle::GetPath(void)
{
	return path->String();
}

/*!
	\brief Converts an ASCII character to Unicode for the style
	\param c An ASCII character
	\return A Unicode value for the character
*/
int16 FontStyle::ConvertToUnicode(uint16 c)
{
	return FT_Get_Char_Index(m_psFace,c);
}

/*!
	\brief Creates a new ServerFont object for the style, given size, shear, and rotation.
	\param size character size in points
	\param rotation rotation in degrees
	\param shear shear (slant) in degrees. 45 <= shear <= 135. 90 is vertical
	\return The new ServerFont object
*/
ServerFont *FontStyle::Instantiate(float size, float rotation, float shear)
{
	ServerFont *f=FindInstance(size, rotation, shear);

	if (NULL == f)
	{
		f=new ServerFont(this, size, rotation, shear);
	}
	else
	{
		f->AddRef();
	}

	return f;
}

ServerFont* FontStyle::FindInstance( float nPointSize, float nRotation, float nShear ) const
{
	std::map<FontProperty,ServerFont*>::const_iterator i;

	__assertw( g_cFontLock.IsLocked() );
	i = m_cInstances.find( FontProperty( nPointSize, nShear, nRotation ) );

	if ( i == m_cInstances.end() )
	{
		return( NULL );
	}

	return( (*i).second );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontStyle::AddInstance( ServerFont* pcInstance )
{
	g_cFontLock.Lock();
	__assertw( g_cFontLock.IsLocked() );
	m_cInstances[ FontProperty( pcInstance->fsize, pcInstance->fshear, pcInstance->frotation ) ] = pcInstance;
	g_cFontLock.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontStyle::RemoveInstance( ServerFont* pcInstance )
{
	g_cFontLock.Lock();

	std::map<FontProperty,ServerFont*>::iterator i;

	__assertw( g_cFontLock.IsLocked() );
	i = m_cInstances.find( FontProperty( pcInstance->fsize, pcInstance->fshear, pcInstance->frotation ) );

	if ( i != m_cInstances.end() )
	{
		m_cInstances.erase( i );
	}
	else
	{
		printf( "Error: FontStyle::RemoveInstance() could not find instance\n" );
	}

	if ( m_bDeleted && m_cInstances.empty() )
	{
		printf( "Last instance of deleted font %s, %s removed. Deleting font\n", family->Name(), Name() );
		delete this;
	}
	g_cFontLock.Unlock();
}


/*!
	\brief Constructor
	\param namestr Name of the family
*/
FontFamily::FontFamily(const char *namestr)
{
	name=new BString(namestr);
	styles=new BList(0);
}


/*!
	\brief Destructor
	
	Deletes all child styles. Note that a FontFamily should not be deleted unless
	its styles have no dependencies or some other really good reason, such as 
	system shutdown.
*/
FontFamily::~FontFamily(void)
{
	delete name;
	
	fontserver->RemoveFamily( Name() );
	styles->MakeEmpty();	// should be empty, but just in case...
	delete styles;
}

/*!
	\brief Returns the name of the family
	\return The family's name
*/
const char *FontFamily::Name(void)
{
	return name->String();
}

/*!
	\brief Adds the style to the family
	\param path full path to the style's font file
	\param face FreeType face handle used to obtain info about the font
*/
void FontFamily::AddStyle(const char *path,FontStyle* pcFont)
{
	if(!path)
		return;
//BUG?
	m_cFonts[pcFont->Name()] = pcFont;
	AddDependent();
}


/*!
	\brief Removes a style from the family and deletes it
	\param style Name of the style to be removed from the family
*/
void FontFamily::RemoveStyle(const char *style)
{
	std::map<std::string,FontStyle*>::iterator i;

	i = m_cFonts.find(style);
	if ( i == m_cFonts.end() )
	{
		printf( "Error: FontFamily::RemoveFont() could not find style '%s' in family '%s'\n", style, Name() );
		return;
	}

	m_cFonts.erase( i );
	if ( m_cFonts.empty() )
	{
		printf( "Font family '%s' is empty. Removing.\n", Name() );
		delete this;
	}
}

/*!
	\brief Returns the number of styles in the family
	\return The number of styles in the family
*/
int32 FontFamily::CountStyles(void)
{
	return styles->CountItems();
}

/*!
	\brief Determines whether the style belongs to the family
	\param style Name of the style being checked
	\return True if it belongs, false if not
*/
bool FontFamily::HasStyle(const char *style)
{
	int32 count=styles->CountItems();
	if(!style || count<1)
		return false;
	FontStyle *fs;
	for(int32 i=0; i<count; i++)
	{
		fs=(FontStyle *)styles->ItemAt(i);
		if(fs && fs->name->Compare(style)==0)
			return true;
	}
	return false;
}

/*! 
	\brief Returns the name of a style in the family
	\param index list index of the style to be found
	\return name of the style or NULL if the index is not valid
*/
const char *FontFamily::GetStyle(int32 index)
{
	FontStyle *fs=(FontStyle*)styles->ItemAt(index);
	if(!fs)
		return NULL;
	return fs->Name();
}

/*!
	\brief Get the FontStyle object for the name given
	\param style Name of the style to be obtained
	\return The FontStyle object or NULL if none was found.
	
	The object returned belongs to the family and must not be deleted.
*/
FontStyle *FontFamily::GetStyle(const char *style)
{
	std::map<std::string,FontStyle*>::iterator i;

	i = m_cFonts.find( style );

	if ( i == m_cFonts.end() )
	{
		return( NULL );
	}

	return( (*i).second );
}



