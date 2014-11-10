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


#include "clipboard.h"

#include <Message.h>
#include <string.h>


SrvClipboard::ClipboardMap SrvClipboard::s_cClipboardMap;


void SrvClipboard::SetData( const char* pzName, char* pData, int nSize )
{
	SrvClipboard* pcClipboard;
	ClipboardMap::iterator i = s_cClipboardMap.find( pzName );

	if ( i == s_cClipboardMap.end() )
	{
		pcClipboard = new SrvClipboard();
		s_cClipboardMap[ pzName ] = pcClipboard;
	}
	else
	{
		pcClipboard = (*i).second;
	}

	delete[] pcClipboard->m_pData;
	pcClipboard->m_pData = new char[nSize];
	pcClipboard->m_nSize = nSize;
	memcpy( pcClipboard->m_pData, pData, nSize );
}


char* SrvClipboard::GetData( const char* pzName, int* pnSize )
{
	ClipboardMap::iterator i = s_cClipboardMap.find( pzName );

	if ( i == s_cClipboardMap.end() )
	{
		return( NULL );
	}
	*pnSize = (*i).second->m_nSize;
	return( (*i).second->m_pData );
}
