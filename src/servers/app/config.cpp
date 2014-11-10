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
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#include "AppServer.h"
#include "Desktop.h"
#include "Layer.h"

#include <String.h>

//---------------------------------------------------------------------------


AppserverConfig* AppserverConfig::s_pcInstance = NULL;

AppserverConfig::AppserverConfig()
{
	s_pcInstance = this;

	const char* pzBaseDir = getenv( "COSMOE_SYS" );
	if( pzBaseDir == NULL )
	{
		pzBaseDir = "/cosmoe";
	}
	
	m_nDoubleClickDelay     = 500000LL;

	AddFontConfig( DEFAULT_FONT_REGULAR, font_properties( "Bitstream Vera Sans", "Roman", FPF_SYSTEM, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_BOLD, font_properties( "Bitstream Vera Sans", "Bold", FPF_SYSTEM, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_FIXED, font_properties( "Bitstream Vera Sans", "Roman", FPF_SYSTEM | FPF_MONOSPACED, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_WINDOW, font_properties( "Bitstream Vera Sans", "Roman", FPF_SYSTEM, 8.0f ) );
	AddFontConfig( DEFAULT_FONT_TOOL_WINDOW, font_properties( "Bitstream Vera Sans", "Roman", FPF_SYSTEM, 7.0f ) );

	m_bDirty = false;
}


AppserverConfig::~AppserverConfig()
{
}


AppserverConfig* AppserverConfig::GetInstance()
{
	return( s_pcInstance );
}


bool AppserverConfig::IsDirty() const
{
	return( m_bDirty );
}


void AppserverConfig::SetConfig( const BMessage* pcConfig )
{
	BString cPath;
	
	if ( pcConfig->FindString( "window_decorator", &cPath ) == 0 )
	{
		SetWindowDecoratorPath( cPath.String() );
	}

	bigtime_t nDelay;
	if ( pcConfig->FindInt64( "doubleclick_delay", &nDelay ) == 0 )
	{
		SetDoubleClickTime( nDelay );
	}

	BMessage cColorConfig;
	if ( pcConfig->FindMessage( "color_config", &cColorConfig ) == 0 )
	{
		for ( int i = 0 ; i < COL_COUNT ; ++i ) {
			int32 sColor;
			if (cColorConfig.FindInt32("color_table", i, &sColor) != 0)
			{
				break;
			}
			__set_default_color(static_cast<default_color_t>(i), _get_rgb_color(sColor));
		}
		m_bDirty = true;
	}
}


void AppserverConfig::GetConfig( BMessage* pcConfig )
{
	pcConfig->AddString( "window_decorator", m_cWindowDecoratorPath.c_str() );
	pcConfig->AddInt64( "doubleclick_delay", m_nDoubleClickDelay );

	BMessage cColorConfig;

	for ( int i = 0 ; i < COL_COUNT ; ++i )
	{
		cColorConfig.AddInt32("color_table",
							_get_uint32_color(get_default_color( static_cast<default_color_t>(i))) );
	}
	pcConfig->AddMessage( "color_config", &cColorConfig );
}




int AppserverConfig::SetWindowDecoratorPath( const std::string& cPath )
{
	if ( m_cWindowDecoratorPath == cPath )
	{
		return( 0 );
	}

	if ( app_server->LoadWindowDecorator( cPath ) == 0 )
	{
		m_cWindowDecoratorPath = cPath;
		m_bDirty = true;
		return( 0 );
	}

	return( 1 );
}


std::string AppserverConfig::GetWindowDecoratorPath() const
{
	return( m_cWindowDecoratorPath );
}


void AppserverConfig::SetDefaultColor( default_color_t eIndex, const rgb_color& sColor )
{
	__set_default_color( eIndex, sColor );
	m_bDirty = true;
}


void AppserverConfig::SetDoubleClickTime( bigtime_t nDelay )
{
	if ( nDelay != m_nDoubleClickDelay )
	{
		m_nDoubleClickDelay = nDelay;
		m_bDirty = true;
	}
}


bigtime_t AppserverConfig::GetDoubleClickTime() const
{
	return( m_nDoubleClickDelay );
}


const font_properties* AppserverConfig::GetFontConfig( const std::string& cName )
{
	std::map<std::string,font_properties>::iterator i = m_cFontSettings.find( cName );
	if ( i == m_cFontSettings.end() )
	{
		return( NULL );
	}

	return( &(*i).second );
}

status_t AppserverConfig::SetFontConfig( const std::string& cName, const font_properties& cProps )
{
	std::map<std::string,font_properties>::iterator i = m_cFontSettings.find( cName );
	if ( i == m_cFontSettings.end() )
	{
		return( -ENOENT );
	}

	(*i).second = cProps;
	m_bDirty = true;
	return( 0 );
}


status_t AppserverConfig::AddFontConfig( const std::string& cName, const font_properties& cProps )
{
	std::map<std::string,font_properties>::iterator i = m_cFontSettings.find( cName );
	if ( i == m_cFontSettings.end() )
	{
		m_cFontSettings.insert( std::pair<std::string,font_properties>(cName, cProps ) );
		return( 0 );
	}

	(*i).second = cProps;
	m_bDirty = true;
	return( 0 );
}


status_t AppserverConfig::DeleteFontConfig( const std::string& cName )
{
	std::map<std::string,font_properties>::iterator i = m_cFontSettings.find( cName );
	if ( i == m_cFontSettings.end() )
	{
		return( -ENOENT );
	}

	m_cFontSettings.erase( i );
	m_bDirty = true;
	return( 0 );
}


static bool match_name( const char* pzName, const char* pzBuffer )
{
	return( strncmp( pzBuffer, pzName, strlen( pzName ) ) == 0 );
}


int AppserverConfig::LoadConfig( FILE* hFile, bool bActivateConfig )
{
	int nLine = 0;

	for (;;)
	{
		char zLineBuf[4096];

		nLine++;

		if ( fgets( zLineBuf, 4096, hFile ) == NULL )
		{
			break;
		}

		if ( zLineBuf[0] == '#' )
		{
			continue;
		}

		bool bEmpty = true;
		for ( int i = 0 ; zLineBuf[i] != 0 ; ++i )
		{
			if ( isspace(zLineBuf[i] ) == false )
			{
				bEmpty = false;
				break;
			}
		}

		if ( bEmpty )
		{
			continue;
		}

		if ( match_name( "DefaultFont:", zLineBuf ) )
		{
			char zName[1024];
			char zFamily[512];
			char zStyle[512];
			uint32 nFlags;
			float  vSize;
			float  vShare;
			float  vRotation;

			if ( sscanf( zLineBuf, "DefaultFont: \"%1023[^\"]\" \"%511[^\"]\" \"%511[^\"]\" %lu %f %f %f\n",
						zName, zFamily, zStyle, &nFlags, &vSize, &vShare, &vRotation ) == 7 ) {
				AddFontConfig( zName, font_properties( zFamily, zStyle, nFlags, vSize, vShare, vRotation ) );
			} else {
				printf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if ( match_name( "WndDecorator", zLineBuf ) )
		{
			char zPath[1024];
			if ( sscanf( zLineBuf, "WndDecorator = %1023s\n", zPath ) == 1 ) {
				if ( bActivateConfig ) {
					SetWindowDecoratorPath( zPath );
				} else {
					m_cWindowDecoratorPath = zPath;
				}
			} else {
				printf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}

		if ( match_name( "DoubleClickDelay", zLineBuf ) )
		{
			bigtime_t nValue;
			if ( sscanf( zLineBuf, "DoubleClickDelay = %Ld\n", &nValue ) == 1 ) {
				m_nDoubleClickDelay = nValue;
			} else {
				printf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}
		if ( match_name( "Color", zLineBuf ) )
		{
			int i,r,g,b,a;
			if ( sscanf( zLineBuf, "Color %d = %d %d %d %d\n", &i, &r, &g, &b, &a ) == 5 ) {
				rgb_color color = {r, g, b, a};
				__set_default_color( default_color_t(i), color );
			} else {
				printf( "Error: Syntax error in appserver config file at line %d\n", nLine );
			}
		}

	}
	m_bDirty = false;
	return( 0 );
}



int AppserverConfig::SaveConfig()
{
	const char* pzBaseDir = getenv( "COSMOE_SYS" );
	if( pzBaseDir == NULL )
	{
		pzBaseDir = "/cosmoe";
	}
	char* pzNewPath = new char[ strlen(pzBaseDir) + 80 ];
	char* pzOldPath = new char[ strlen(pzBaseDir) + 80 ];
	strcpy( pzNewPath, pzBaseDir );
	strcat( pzNewPath, "/config/appserver.new" );
	strcpy( pzOldPath, pzBaseDir );
	strcat( pzOldPath, "/config/appserver" );
	FILE* hFile = fopen( pzNewPath, "w" );


	fprintf( hFile, "\n" );

	std::map<std::string,font_properties>::iterator cIterator;

	for ( cIterator = m_cFontSettings.begin() ; cIterator != m_cFontSettings.end() ; ++cIterator )
	{
		fprintf( hFile, "DefaultFont: \"%s\" \"%s\" \"%s\" %08x %.2f %.2f %.2f\n", (*cIterator).first.c_str(),
				(*cIterator).second.m_cFamily.c_str(), (*cIterator).second.m_cStyle.c_str(), (*cIterator).second.m_nFlags,
				(*cIterator).second.m_vSize, (*cIterator).second.m_vShear, (*cIterator).second.m_vRotation );
	}


	fprintf( hFile, "\n" );

	fprintf( hFile, "WndDecorator     = %s\n", m_cWindowDecoratorPath.c_str() );
	fprintf( hFile, "DoubleClickDelay = %Ld\n", m_nDoubleClickDelay );

	fprintf( hFile, "\n" );

	for ( int i = 0 ; i < COL_COUNT ; ++i )
	{
		rgb_color sColor = get_default_color( default_color_t(i) );
		fprintf( hFile, "Color %02d = %03d %03d %03d %03d\n", i, sColor.red, sColor.green, sColor.blue, sColor.alpha );
	}

	fclose( hFile );

	rename( pzNewPath, pzOldPath );
	m_bDirty = false;
	delete[] pzOldPath;
	delete[] pzNewPath;
	return( 0 );
}
