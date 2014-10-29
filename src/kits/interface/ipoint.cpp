//------------------------------------------------------------------------------
//	Copyright (c) 2003, Bill Hayden
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ipoint.cpp
//	Author:			Bill Hayden (hayden@haydentech.com)
//------------------------------------------------------------------------------


// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <SupportDefs.h>
#include <IPoint.h>


IPoint	IPoint::operator-(void) const
{
	return IPoint(-x, -y);
}

//------------------------------------------------------------------------------

IPoint	IPoint::operator+(const IPoint& pt) const
{
	return IPoint(x + pt.x, y + pt.y);
}

//------------------------------------------------------------------------------

IPoint	IPoint::operator-(const IPoint& pt) const
{
	return IPoint(x - pt.x, y - pt.y);
}

//------------------------------------------------------------------------------

const IPoint&	IPoint::operator+=(const IPoint& pt)
{
	x += pt.x;
	y += pt.y;
	return( *this );
}

//------------------------------------------------------------------------------

const IPoint&	IPoint::operator-=(const IPoint& pt)
{
	x -= pt.x;
	y -= pt.y;
	return( *this );
}

//------------------------------------------------------------------------------

bool	IPoint::operator<(const IPoint& pt) const
{
	return( y < pt.y || ( y == pt.y && x < pt.x ) );
}

//------------------------------------------------------------------------------

bool	IPoint::operator>(const IPoint& pt) const
{
	return( y > pt.y || ( y == pt.y && x > pt.x ) );
}

//------------------------------------------------------------------------------

bool	IPoint::operator==(const IPoint& pt) const
{
	return( y == pt.y && x == pt.x );
}

//------------------------------------------------------------------------------

bool	IPoint::operator!=(const IPoint& pt) const
{
	return( y != pt.y || x != pt.x );
}

