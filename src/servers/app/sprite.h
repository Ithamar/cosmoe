/*
 *  The Cosmoe application server
 *  Copyright (C) 1999 Kurt Skauen
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


#ifndef __F_SRVSPRITE_H__
#define __F_SRVSPRITE_H__

#include <vector>
#include <Region.h>
#include <Locker.h>
#include <IRect.h>




class ServerBitmap;

class SrvSprite
{
public:
	SrvSprite( const IRect& cFrame, const IPoint& cPos, const IPoint& cHotSpot,
			ServerBitmap* pcTarget, ServerBitmap* pcImage );
	~SrvSprite();

	IRect GetBounds() const { return( m_cBounds ); }
	void Draw();
	void Erase();

	void Draw( ServerBitmap* pcTarget, const IPoint& cPos );
	void Capture( ServerBitmap* pcTarget, const IPoint& cPos );
	void Erase( ServerBitmap* pcTarget, const IPoint& cPos );

	void MoveBy( const IPoint& cDelta );
	void MoveTo( const IPoint& cNewPos ) { MoveBy( cNewPos - m_cPosition ); }

	static void ColorSpaceChanged();
	static void Hide( const IRect& cFrame );
	static void Hide();
	static void Unhide();

private:
	static std::vector<SrvSprite*> s_cInstances;
	static int32                   s_nHideCount;
	static BLocker                 s_cLock;

	IPoint     m_cPosition;
	IPoint     m_cHotSpot;
	ServerBitmap* m_pcImage;
	ServerBitmap* m_pcTarget;
	ServerBitmap* m_pcBackground;
	IRect      m_cBounds;
	bool       m_bVisible;
};


#endif // __F_SRVSPRITE_H__
