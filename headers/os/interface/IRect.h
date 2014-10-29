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


#ifndef	__IRECT_H__
#define	__IRECT_H__


#include <IPoint.h>
#include <Rect.h>

 /**
 * \ingroup interface
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx), Bill Hayden (hayden@haydentech.com)
 *****************************************************************************/

class IRect
{
public:
	int		left;
	int		top;
	int		right;
	int		bottom;

	IRect( void )	{ left = top = 999999;	right = bottom = -999999;	}
	IRect( int l, int t, int r, int b )	{ left = l; top = t; right = r; bottom = b;	}
	IRect( const IPoint& cMin, const IPoint& cMax )	{ left = cMin.x; top = cMin.y; right = cMax.x; bottom = cMax.y;	}
	IRect( const BRect& cRect );
	~IRect() {}

	BRect	AsBRect()	{ return BRect(left, top, right, bottom); } const

	bool	IsValid() const		{ return( left <= right && top <= bottom );	}
	void	Invalidate( void )	{ left = top = 999999; right = bottom = -999999; }
	bool	Contains( const IPoint& cPoint ) const
	{ return( !( cPoint.x < left || cPoint.x > right || cPoint.y < top || cPoint.y > bottom ) ); }

	bool Contains( const IRect& cRect ) const
	{ return( !( cRect.right < left || cRect.left > right || cRect.bottom < top || cRect.top > bottom ) ); }

	int	   Width() const		{ return( right - left ); }
	int	   Height() const		{ return( bottom - top ); }
	IPoint Size() const 		{ return( IPoint( right - left, bottom - top ) ); }
	IPoint LeftTop() const		{ return( IPoint( left, top ) );	}
	IPoint RightBottom() const	{ return( IPoint( right, bottom ) );	}
	IRect  Bounds( void ) const	{ return( IRect( 0, 0, right - left, bottom - top ) );		}
	IRect&	Resize( int nLeft, int nTop, int nRight, int nBottom ) {
	left += nLeft; top += nTop; right += nRight; bottom += nBottom;
	return( *this );
	}
	IRect operator+( const IPoint& cPoint ) const
	{ return( IRect( left + cPoint.x, top + cPoint.y, right + cPoint.x, bottom + cPoint.y ) ); }
	IRect operator-( const IPoint& cPoint ) const
	{ return( IRect( left - cPoint.x, top - cPoint.y, right - cPoint.x, bottom - cPoint.y ) ); }

	IPoint operator+( const IRect& cRect ) const	{ return( IPoint( left + cRect.left, top + cRect.top ) ); }
	IPoint operator-( const IRect& cRect ) const	{ return( IPoint( left - cRect.left, top - cRect.top ) ); }

	IRect operator&( const IRect& cRect ) const
	{ return( IRect( _max( left, cRect.left ), _max( top, cRect.top ), _min( right, cRect.right ), _min( bottom, cRect.bottom ) ) ); }
	void 	operator&=( const IRect& cRect )
	{ left = _max( left, cRect.left ); top = _max( top, cRect.top );
	right = _min( right, cRect.right ); bottom = _min( bottom, cRect.bottom ); }
	IRect operator|( const IRect& cRect ) const
	{ return( IRect( _min( left, cRect.left ), _min( top, cRect.top ), _max( right, cRect.right ), _max( bottom, cRect.bottom ) ) ); }
	void 	operator|=( const IRect& cRect )
	{ left = _min( left, cRect.left ); top = _min( top, cRect.top );
	right = _max( right, cRect.right ); bottom = _max( bottom, cRect.bottom ); }
	IRect operator|( const IPoint& cPoint ) const
	{ return( IRect( _min( left, cPoint.x ), _min( top, cPoint.y ), _max( right, cPoint.x ), _max( bottom, cPoint.y ) ) ); }
	void operator|=( const IPoint& cPoint )
	{ left = _min( left, cPoint.x ); top =  _min( top, cPoint.y ); right = _max( right, cPoint.x ); bottom = _max( bottom, cPoint.y ); }


	void operator+=( const IPoint& cPoint ) { left += cPoint.x; top += cPoint.y; right += cPoint.x; bottom += cPoint.y; }
	void operator-=( const IPoint& cPoint ) { left -= cPoint.x; top -= cPoint.y; right -= cPoint.x; bottom -= cPoint.y; }

	bool operator==( const IRect& cRect ) const
	{ return( left == cRect.left && top == cRect.top && right == cRect.right && bottom == cRect.bottom ); }

	bool operator!=( const IRect& cRect ) const
	{ return( left != cRect.left || top != cRect.top || right != cRect.right || bottom != cRect.bottom ); }

private:
	int _min( int a, int b ) const			{ return( (a<b) ? a : b ); }
	int _max( int a, int b ) const			{ return( (a>b) ? a : b ); }
};

#endif
