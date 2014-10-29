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

#ifndef	__IPOINT_H__
#define	__IPOINT_H__

#include <Point.h>

/**
 * \ingroup interface
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx), Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

class IPoint
{
public:
    int x;
    int y;

					IPoint()			 			{ x = y = 0;			  }
					IPoint( const IPoint& cPnt )	{ x = cPnt.x; y = cPnt.y; }
					IPoint( int nX, int nY )		{ x = nX; y = nY;		  }
    explicit inline IPoint( const BPoint& cPnt );

	BPoint			AsBPoint() { return BPoint(x,y); }

    IPoint			operator-( void ) const;
    IPoint			operator+( const IPoint& cPoint ) const;
    IPoint			operator-( const IPoint& cPoint ) const;
    const IPoint&	operator+=( const IPoint& cPoint );
    const IPoint&	operator-=( const IPoint& cPoint );
    bool			operator<( const IPoint& cPoint ) const;
    bool			operator>( const IPoint& cPoint ) const;
    bool			operator==( const IPoint& cPoint ) const;
    bool			operator!=( const IPoint& cPoint ) const;
};


IPoint::IPoint( const BPoint& cPnt )
{
	x = int(cPnt.x);
	y = int(cPnt.y);
}

#endif
