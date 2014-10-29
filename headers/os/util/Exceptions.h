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


#ifndef __UTIL_EXCEPTIONS_H__
#define __UTIL_EXCEPTIONS_H__

#include <string.h>
#include <errno.h>

#include <exception>


class errno_exception : public std::exception
{
public:
	errno_exception( const std::string& cMessage, int nErrorCode ) : m_cMessage(cMessage)
	{
		m_nErrorCode = nErrorCode;
	}
	
	errno_exception( const std::string& cMessage ) : m_cMessage(cMessage)
	{
		m_nErrorCode = errno;
	}
	
	~errno_exception() throw() {};
	
	virtual const char*	what() const throw()	{ return( m_cMessage.c_str() ); }
	char*				error_str() const		{ return( strerror( m_nErrorCode ) ); }
	int					error() const			{ return( m_nErrorCode ); }
	
private:
	std::string	m_cMessage;
	int			m_nErrorCode;
};



#endif // __UTIL_EXCEPTIONS_H__
