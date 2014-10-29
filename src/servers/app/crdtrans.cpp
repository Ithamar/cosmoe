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


#include "Layer.h"




BPoint Layer::ConvertToParent( const BPoint& cPoint ) const
{
	return( cPoint + GetLeftTop() );
}


void Layer::ConvertToParent( BPoint* pcPoint ) const
{
	*pcPoint += GetLeftTop();
}


void Layer::ConvertToParent( BRect* pcRect ) const
{
	pcRect->OffsetBy(GetLeftTop());
}


BPoint Layer::ConvertFromParent( const BPoint& cPoint ) const
{
	return( cPoint - GetLeftTop() );
}


void Layer::ConvertFromParent( BPoint* pcPoint ) const
{
	*pcPoint -= GetLeftTop();
}


void Layer::ConvertFromParent( BRect* pcRect ) const
{
	pcRect->OffsetBy(-GetLeftTop());
}


BPoint Layer::ConvertToRoot( const BPoint& cPoint ) const
{
	if ( fParent != NULL )
	{
		return( fParent->ConvertToRoot( cPoint + GetLeftTop() ) );
	}

	return( cPoint );
}


void Layer::ConvertToRoot( BPoint* pcPoint ) const
{
	if ( fParent != NULL )
	{
		*pcPoint += GetLeftTop();
		fParent->ConvertToRoot( pcPoint );
	}
}


void Layer::ConvertToRoot( BRect* pcRect ) const
{
	if ( fParent != NULL )
	{
		pcRect->OffsetBy(GetLeftTop());
		fParent->ConvertToRoot( pcRect );
	}
}


BPoint Layer::ConvertFromRoot( const BPoint& cPoint ) const
{
	if ( fParent != NULL ) {
		return( fParent->ConvertFromRoot( cPoint - GetLeftTop() ) );
	}
	return( cPoint );
}


void Layer::ConvertFromRoot( BPoint* pcPoint ) const
{
	if ( fParent != NULL )
	{
		*pcPoint -= GetLeftTop();
		fParent->ConvertFromRoot( pcPoint );
	}
}


void Layer::ConvertFromRoot( BRect* pcRect ) const
{
	if ( fParent != NULL )
	{
		pcRect->OffsetBy(-GetLeftTop());
		fParent->ConvertFromRoot( pcRect );
	}
}
