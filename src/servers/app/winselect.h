/*
 *  The Cosmoe application server
 *  Copyright (C) 1999  Kurt Skauen
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

#ifndef __F_WINSELECT_H__
#define __F_WINSELECT_H__

#include "Layer.h"

#include <vector>



class ServerWindow;

class WinSelect : public Layer
{
public:
			WinSelect();
	virtual ~WinSelect();

	void UpdateWinList( bool bMoveToFront, bool bSetFocus );
	virtual void RequestDraw( const IRect& r, bool bUpdate );
	void Step( bool bForward );

private:
	int        m_nCurSelect;
	std::vector<ServerWindow*> m_cWindows;
	ServerWindow*                  m_pcOldFocusWindow;
};



#endif // __F_WINSELECT_H__
