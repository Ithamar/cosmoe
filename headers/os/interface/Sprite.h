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


#ifndef __SPRITE_H__
#define __SPRITE_H__

#include <Region.h>




class BBitmap;

class Sprite
{
public:
			Sprite( const BPoint& cPosition, BBitmap* pcBitmap );
			~Sprite();

	void	MoveBy( const BPoint& cDelta );
	void	MoveTo( const BPoint& cNewPos );

private:
	uint32	m_nHandle;
	BPoint	m_cPosition;
};


#endif // __SPRITE_H__

