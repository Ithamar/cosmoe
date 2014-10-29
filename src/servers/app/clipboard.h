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

#ifndef __CLIPBOARD_H__
#define __CLIPBOARD_H__

#include <SupportDefs.h>
#include <map>
#include <string>

class SrvClipboard
{
public:
	static void			SetData( const char* pzName, char* pzData, int nSize );
	static char*		GetData( const char* pzName, int* pnSize );

private:
						SrvClipboard() { m_pData = NULL; }

	typedef std::map<std::string,SrvClipboard*> ClipboardMap;

	static ClipboardMap	s_cClipboardMap;
	char*				m_pData;
	int					m_nSize;
};


#endif // __CLIPBOARD_H__
