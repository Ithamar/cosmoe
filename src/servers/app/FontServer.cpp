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
//	File Name:		FontServer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handles the largest part of the font subsystem
//  
//------------------------------------------------------------------------------
#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <storage/Path.h>	// specified to be able to build under Dano
#include <File.h>
#include <Message.h>
#include <String.h>

#include <FontServer.h>
#include <FontFamily.h>
#include <ServerFont.h>
#include "ServerConfig.h"

#include <errno.h>
#include <Debug.h>
#include <macros.h>

void pathcat( char* pzPath, const char* pzName );

extern FTC_Manager ftmanager; 
FT_Library ftlib;
FontServer *fontserver=NULL;

//#define PRINT_FONT_LIST

/*!
	\brief Access function to request a face via the FreeType font cache
*/
static FT_Error face_requester(FTC_FaceID face_id, FT_Library library,
	FT_Pointer request_data, FT_Face *aface)
{ 
	CachedFace face = (CachedFace) face_id;
	return FT_New_Face(ftlib,face->file_path.String(),face->face_index,aface); 
} 

//! Does basic set up so that directories can be scanned
FontServer::FontServer(void)
{
	lock=create_sem(1,"fontserver_lock");
	init=(FT_Init_FreeType(&ftlib)==0)?true:false;
	if ( init != true )
	{
		printf( "ERROR: While initializing font renderer, code = %d\n", init );
	}

/*
	Fire up the font caching subsystem.
	The three zeros tell FreeType to use the defaults, which are 2 faces,
	4 face sizes, and a maximum of 200000 bytes. I will probably change
	these numbers in the future to maximize performance for your "average"
	application.
*/
	if(FTC_Manager_New(ftlib,0,0,0,&face_requester,NULL,&ftmanager)!=0
				&& init)
		init=false;
		
	families=new BList(0);
	plain=NULL;
	bold=NULL;
	fixed=NULL;
}

//! Frees items allocated in the constructor and shuts down FreeType
FontServer::~FontServer(void)
{
	delete_sem(lock);
	delete families;
	FTC_Manager_Done(ftmanager);
	FT_Done_FreeType(ftlib);
}

//! Locks access to the font server
bool FontServer::Lock(void)
{
	return( g_cFontLock.Lock() );	// NLS
}

//! Unlocks access to the font server
void FontServer::Unlock(void)
{
	g_cFontLock.Unlock();
}

/*!
	\brief Counts the number of font families available
	\return The number of unique font families currently available
*/
int32 FontServer::CountFamilies(void)
{
	int nCount;

	g_cFontLock.Lock();
	nCount = families->CountItems();
	g_cFontLock.Unlock();
	return nCount;
}

/*!
	\brief Counts the number of styles available in a font family
	\param family Name of the font family to scan
	\return The number of font styles currently available for the font family
*/
int32 FontServer::CountStyles(const char *family)
{
	int nCount = -1;

	g_cFontLock.Lock();
	FontFamily *f=_FindFamily(family);
	if(f)
	{
		nCount = f->m_cFonts.size();
	}

	g_cFontLock.Unlock();
	return( nCount );
}

/*!
	\brief Removes a font family from the font list
	\param family The family to remove
*/
void FontServer::RemoveFamily(const char *family)
{
	FontFamily *f=_FindFamily(family);
	if(f)
	{
		families->RemoveItem(f);
		delete f;
	}
}

int FontServer::GetFamily( int nIndex, char* pzFamily, uint32* pnFlags )
{
	int nError = EINVAL;
	g_cFontLock.Lock();

	FontFamily* f = (FontFamily*)families->ItemAt(nIndex);
	if (f)
	{
		strcpy(pzFamily, f->Name());
		nError = 0;
	}
	
	g_cFontLock.Unlock();
	return nError;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int FontServer::GetStyle( const std::string& cFamily, int nIndex, char* pzStyle, uint32* pnFlags )
{
	int nError = 0;

	g_cFontLock.Lock();

	FontFamily* pcFamily = _FindFamily( cFamily.c_str() );

	if ( pcFamily != NULL )
	{
		if ( nIndex < int(pcFamily->m_cFonts.size()) )
		{
			std::map<std::string,FontStyle*>::const_iterator i = pcFamily->m_cFonts.begin();

			while( nIndex-- > 0 ) ++i;

			strcpy( pzStyle, (*i).first.c_str() );
			*pnFlags = 0;
			if ( (*i).second->IsFixedWidth() )
			{
				*pnFlags |= B_IS_FIXED;
			}
			if ( (*i).second->IsScalable() )
			{
				*pnFlags |= FONT_IS_SCALABLE;
				if ( (*i).second->GetBitmapSizes().size() > 0 )
				{
					*pnFlags |= B_HAS_TUNED_FONT;
				}
			}
			else
			{
				*pnFlags |= B_HAS_TUNED_FONT;
			}
			nError = 0;
		}
		else
		{
			nError = EINVAL;
		}
	}
	else
	{
		nError = ENOENT;
	}
	g_cFontLock.Unlock();

	return( nError );
}

/*!
	\brief Protected function which locates a FontFamily object
	\param name The family to find
	\return Pointer to the specified family or NULL if not found.
	
	Do NOT delete the FontFamily returned by this function.
*/
FontFamily *FontServer::_FindFamily(const char *name)
{
	if(!init)
		return NULL;
	int32 count=families->CountItems(), i;
	FontFamily *family;
	for(i=0; i<count; i++)
	{
		family=(FontFamily*)families->ItemAt(i);
		if(strcmp(family->Name(),name)==0)
			return family;
	}
	return NULL;
}


/*!
	\brief Scan a folder for all valid fonts
	\param fontspath Path of the folder to scan.
	\return 
	- \c B_OK				Success
	- \c B_NAME_TOO_LONG	The path specified is too long
	- \c B_ENTRY_NOT_FOUND	The path does not exist
	- \c B_LINK_LIMIT		A cyclic loop was detected in the file system
	- \c B_BAD_VALUE		Invalid input specified
	- \c B_NO_MEMORY		Insufficient memory to open the folder for reading
	- \c B_BUSY				A busy node could not be accessed
	- \c B_FILE_ERROR		An invalid file prevented the operation.
	- \c B_NO_MORE_FDS		All file descriptors are in use (too many open files). 
*/
status_t FontServer::ScanDirectory(const char *fontspath)
{
	// This bad boy does all the real work. It loads each entry in the
	// directory. If a valid font file, it adds both the family and the style.
	// Both family and style are stored internally as BStrings. Once everything

	int32 validcount=0;
    FT_Face  face;
    FT_Error error;
    FT_CharMap charmap;
    FontFamily *family;

	DIR*        hDir;
	dirent*        psEntry;

	printf( "FontServer::ScanDirectory(): opening %s\n", fontspath );

	if ( (hDir = opendir( fontspath ) ) )
	{
		while( (psEntry = readdir( hDir )) )
		{
			printf( "FontServer::ScanDirectory(): found entry %s\n", psEntry->d_name );
			char     zFullPath[ PATH_MAX ];

			if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}
			strcpy( zFullPath, fontspath );
			pathcat( zFullPath, psEntry->d_name );

			error=FT_New_Face(ftlib, zFullPath,0,&face);

			if (error!=0)
				continue;

			charmap=_GetSupportedCharmap(face);
			if(!charmap)
			{
				FT_Done_Face(face);
				continue;
			}

			face->charmap=charmap;

			printf( "family_name = %s\nstyle_name = %s\n", face->family_name, face->style_name );

			family=_FindFamily(face->family_name );

			if (!family)
			{
				#ifdef PRINT_FONT_LIST
				printf("Font Family: %s\n",face->family_name);
				#endif

				family=new FontFamily(face->family_name);
				families->AddItem(family);
			}

			if(family->GetStyle(face->style_name))
			{
				FT_Done_Face(face);
				continue;
			}

			#ifdef PRINT_FONT_LIST
			printf("\tFont Style: %s\n",face->style_name);
			#endif

			// Has vertical metrics?
			FontStyle* pcFont = new FontStyle(zFullPath, family, face);
			validcount++;
			
			__assertw( NULL != pcFont );
		}
		printf( "Directory '%s' scanned, %ld fonts found\n", fontspath, validcount );
		closedir( hDir );
	}

	need_update=true;
	return B_OK;
}


/*!
	\brief Finds and returns the first valid charmap in a font
	
	\param face Font handle obtained from FT_Load_Face()
	\return An FT_CharMap or NULL if unsuccessful
*/
FT_CharMap FontServer::_GetSupportedCharmap(const FT_Face &face)
{
	int32 i;
	FT_CharMap charmap;
	for(i=0; i< face->num_charmaps; i++)
	{
		charmap=face->charmaps[i];
		switch(charmap->platform_id)
		{
			case 3:
			{
				// if Windows Symbol or Windows Unicode
				if(charmap->encoding_id==0 || charmap->encoding_id==1)
					return charmap;
				break;
			}
			case 1:
			{
				// if Apple Unicode
				if(charmap->encoding_id==0)
					return charmap;
				break;
			}
			case 0:
			{
				// if Apple Roman
				if(charmap->encoding_id==0)
					return charmap;
				break;
			}
			default:
			{
				break;
			}
		}
	}
	return NULL;

}

/*!
	\brief This saves all family names and styles to the file specified in
	ServerConfig.h as SERVER_FONT_LIST as a flattened BMessage.

	This operation is not done very often because the access to disk adds a significant 
	performance hit.

	The format for storage consists of two things: an array of strings with the name 'family'
	and a number of small string arrays which have the name of the font family. These are
	the style lists. 

	Additionally, any fonts which have bitmap strikes contained in them or any fonts which
	are fixed-width are named in the arrays 'tuned' and 'fixed'.
*/
void FontServer::SaveList(void)
{
}


ServerFont* FontServer::OpenInstance( const std::string& cFamily, const std::string& cStyle,
                                             float nPointSize, float nRotation, float nShare )
{
	FontFamily*        pcFamily;

	g_cFontLock.Lock();

	if ( (pcFamily = _FindFamily( cFamily.c_str() )) )
	{
		FontStyle*        pcFont = pcFamily->GetStyle( cStyle.c_str() );

		if ( pcFont != NULL )
		{
			ServerFont* pcInstance= pcFont->Instantiate( nPointSize, nRotation, nShare );

			if ( pcInstance != NULL )
			{
				g_cFontLock.Unlock();
				return( pcInstance );
			}
		}
	}
	
	g_cFontLock.Unlock();
	
	return( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontStyle* FontServer::OpenFont( const std::string& cFamily, const std::string& cStyle )
{
	FontFamily* pcFamily;
	FontStyle*      pcFont = NULL;

	if ( Lock() )	// NLS (not changed, just verified)
	{
		if ( (pcFamily = _FindFamily( cFamily.c_str() )) )
		{
			pcFont = pcFamily->GetStyle( cStyle.c_str() );
		}
		Unlock();
	}
	return( pcFont );
}

void pathcat( char* pzPath, const char* pzName )
{
	int nPathLen = strlen( pzPath );

	if ( nPathLen > 0 )
	{
		if ( pzPath[ nPathLen - 1 ] != '/' )
		{
			pzPath[ nPathLen ] = '/';
			nPathLen++;
		}
	}
	strcpy( pzPath + nPathLen, pzName );
}

