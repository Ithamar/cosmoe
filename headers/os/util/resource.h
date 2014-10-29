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


#ifndef	__RESOURCE_H__
#define	__RESOURCE_H__

/** 
 * \ingroup util
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx), Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

class	Resource
{
public:
			Resource()	{ m_nRefCount = 1; }

	void	AddRef( void )				{ m_nRefCount++;	}
	void	Release( void )				{ if ( --m_nRefCount == 0 ) delete this; }
	int		GetRefCount( void ) const 	{ return( m_nRefCount ); }

protected:
	virtual	~Resource()					{}
	
private:
	int		m_nRefCount;
};


#endif	//	__RESOURCE_H__
