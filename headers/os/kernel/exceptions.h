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

#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#include <exception>
#include <string.h>

class GeneralFailure : public std::exception
{
public:
	GeneralFailure( const char* pzMessage, int nErrorCode )
	{
		strncpy( m_zMessage, pzMessage, 256 );
		m_zMessage[255] = '\0';
		m_nErrorCode = nErrorCode;
	}
	
	virtual const char*	what() const throw() { return( m_zMessage ); }
	int					code() const { return( m_nErrorCode ); }
	
private:
	char				m_zMessage[256];
	int					m_nErrorCode;
};

#endif // __F_GUI_EXCEPTIONS_H__

