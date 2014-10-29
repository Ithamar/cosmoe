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

#include <float.h>

#include <Debug.h>

#include "fontnode.h"
#include "ServerFont.h"
#include "FontServer.h"
#include "FontFamily.h"
#include "config.h"

#include <Messenger.h>



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontNode::FontNode() : m_cDependenciesMutex( "fnode_dep_mutex" )
{
    m_pcFont        = NULL;
    m_pcInstance= NULL;

    m_nSize        = 63;
    m_nShear        = 0;
    m_nRotation        = 0;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontNode::~FontNode()
{
    if ( m_pcInstance != NULL ) {
        m_pcInstance->Release();
    }
}

Glyph* FontNode::GetGlyph( int nChar )
{
    if ( m_pcFont != NULL ) {
        return( m_pcInstance->GetGlyph( m_pcInstance->CharToUnicode( nChar ) ) );
    } else {
        return( NULL );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t FontNode::SetFamilyAndStyle( const std::string& cFamily, const std::string& cStyle )
{
    g_cFontLock.Lock();
    if ( m_pcInstance != NULL ) {
        if ( m_pcFont == NULL ) {
            printf( "Error: FontNode::SetFamilyAndStyle() FontNode have font-instance but no font\n" );
        }
        m_pcInstance->Release();
        m_pcInstance = NULL;
    }

    font_properties sProp;

    if ( strcmp( cFamily.c_str(), SYS_FONT_FAMILY ) == 0 ) {
        if ( strcmp( cStyle.c_str(), SYS_FONT_FIXED ) == 0 ) {
            sProp = *AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_FIXED );
            g_cFontLock.Unlock();
            return( SetFamilyAndStyle( sProp.m_cFamily, sProp.m_cStyle ) );
        }
        if ( strcmp( cStyle.c_str(), SYS_FONT_PLAIN ) == 0 ) {
            sProp = *AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_REGULAR );
            g_cFontLock.Unlock();
            return( SetFamilyAndStyle( sProp.m_cFamily, sProp.m_cStyle ) );
        }
        if ( strcmp( cStyle.c_str(), SYS_FONT_BOLD ) == 0 ) {
            sProp = *AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_BOLD );
            g_cFontLock.Unlock();
            return( SetFamilyAndStyle( sProp.m_cFamily, sProp.m_cStyle ) );
        }
    }
    m_pcFont = fontserver->OpenFont( cFamily, cStyle );

    if ( NULL != m_pcFont ) {
        if ( m_pcFont->IsScalable() == false ) {
            float vSize = float(m_nSize)/64.0f;
            SnapPointSize( &vSize );
            m_nSize = int(vSize*64.0f);
        }
        m_pcInstance = m_pcFont->Instantiate( m_nSize, m_nRotation, m_nShear );
        g_cFontLock.Unlock();
        return( 0 );
    }
    printf( "Error : FontNode::SetFamilyAndStyle() failed to open font\n" );
    g_cFontLock.Unlock();
    return( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t FontNode::SetProperties( float nSize, float nShear, float nRotation )
{
	g_cFontLock.Lock();

	if ( nSize == m_nSize && nShear == m_nShear && nRotation == m_nRotation ) {
		g_cFontLock.Unlock();
		return( 0 );
	}

	if ( m_pcInstance != NULL ) {
		if ( m_pcInstance->GetRefCount() > 1 ) {
			m_pcInstance->Release();
			m_pcInstance = m_pcFont->Instantiate( nSize, nShear, nRotation );
		} else {
			ServerFont* pcInstance = m_pcFont->FindInstance( nSize, nRotation, nShear );
			if ( pcInstance != NULL ) {
				m_pcInstance->Release();
				m_pcInstance = pcInstance;
				m_pcInstance->AddRef();
			} else {
				m_pcInstance->SetProperties( nSize, nShear, nRotation );
			}
		}
	}

	if ( NULL != m_pcInstance ) {
		m_nSize                = nSize;
		m_nShear        = nShear;
		m_nRotation        = nRotation;

		g_cFontLock.Unlock();
		return( 0 );
	} else {
		printf( "ERROR : FontNode::SetProperties() called on node without an instance\n" );
		g_cFontLock.Unlock();
		return( -1 );
	}
}

status_t FontNode::SetProperties( const font_properties& sProps )
{
	g_cFontLock.Lock();
	if ( NULL != m_pcInstance )
	{
		if ( m_pcFont == NULL ) {
			printf( "Error: FontNode::SetProperties() FontNode have font-instance but no font\n" );
		}
		m_pcInstance->Release();
		m_pcInstance = NULL;
	}

	m_nSize     = int(sProps.m_vSize*64.0f);
	m_nShear    = int(sProps.m_vShear*64.0f);
	m_nRotation = int(sProps.m_vRotation*64.0f);
	
	m_pcFont = fontserver->OpenFont( sProps.m_cFamily, sProps.m_cStyle );

	if ( NULL != m_pcFont ) {
		if ( m_pcFont->IsScalable() == false ) {
			float vSize = float(m_nSize)/64.0f;
			SnapPointSize( &vSize );
			m_nSize = int(vSize*64.0f);
		}
		m_pcInstance = m_pcFont->Instantiate( m_nSize, m_nRotation, m_nShear );
		g_cFontLock.Unlock();
		return( 0 );
	}
	printf( "Error : FontNode::SetProperties() failed to open font\n" );
	g_cFontLock.Unlock();
	return( -1 );
}

void FontNode::SnapPointSize( float* pvSize ) const
{
	if ( m_pcFont == NULL || m_pcFont->IsScalable() ) {
		return;
	}
	const BFont::size_list_t& cSizeList = m_pcFont->GetBitmapSizes();
	float vSize     = *pvSize;
	float vClosest  = vSize;
	float vMinDelta = FLT_MAX;
	
	for ( uint i = 0 ; i < cSizeList.size() ; ++i ) {
		float vDelta = fabs( cSizeList[i] - vSize );
		if ( vDelta < vMinDelta ) {
			vMinDelta = vDelta;
			vClosest = cSizeList[i];
		}
	}
	*pvSize = vClosest;
}

FontNode::DependencyList_t::iterator FontNode::AddDependency( BMessenger* pcTarget, void* pView )
{
    DependencyList_t::iterator i;

    m_cDependenciesMutex.Lock();
    i = m_cDependencies.insert( m_cDependencies.end(), std::make_pair( pcTarget, pView ) );
    m_cDependenciesMutex.Unlock();
    return( i );
}

void FontNode::RemoveDependency( DependencyList_t::iterator& cIterator )
{
    m_cDependenciesMutex.Lock();
    m_cDependencies.erase( cIterator );
    m_cDependenciesMutex.Unlock();
}

void FontNode::NotifyDependent() const
{
    m_cDependenciesMutex.Lock();
    for ( DependencyList_t::const_iterator i = m_cDependencies.begin() ; i != m_cDependencies.end() ; ++i ) {
        try {
            BMessage cMsg( M_FONT_CHANGED );
            cMsg.AddPointer( "_widget", (*i).second ); 
            (*i).first->SendMessage( &cMsg );
        } catch (...) {
        }
    }
    m_cDependenciesMutex.Unlock();
}
