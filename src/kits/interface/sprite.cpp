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


#include <assert.h>

#include <Sprite.h>
#include <Bitmap.h>
#include <Application.h>
#include <exceptions.h>




Sprite::Sprite( const BPoint& cPosition, BBitmap* pcBitmap ) : m_cPosition( cPosition )
{
	assert( be_app != NULL );

	int nError = be_app->CreateSprite( BRect(cPosition.x,cPosition.y,cPosition.x,cPosition.y), pcBitmap->fToken, &m_nHandle );

	if ( nError < 0 )
	{
		throw( GeneralFailure( "Failed to create message port", -nError ) );
	}
}


Sprite::~Sprite()
{
	assert( be_app != NULL );
	be_app->DeleteSprite( m_nHandle );
}


void Sprite::MoveTo( const BPoint& cNewPos )
{
	assert( be_app != NULL );
	be_app->MoveSprite( m_nHandle, cNewPos );
	m_cPosition = cNewPos;
}


void Sprite::MoveBy( const BPoint& cPos )
{
	MoveTo( m_cPosition + cPos );
}
