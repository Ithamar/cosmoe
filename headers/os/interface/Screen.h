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


#ifndef _SCREEN_H
#define _SCREEN_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>
#include <IPoint.h>


/*----------------------------------------------------------------*/
/*----- BScreen structures and declarations ----------------------*/

class BWindow;
class BPrivateScreen;

/*----------------------------------------------------------------*/
/*----- BScreen class --------------------------------------------*/

class BScreen {
public:  
        BScreen( screen_id id=B_MAIN_SCREEN_ID );
        BScreen( BWindow *win );
        ~BScreen();

        bool   			IsValid()	{ return true; }

        color_space		ColorSpace();
        BRect			Frame();
        screen_id		ID() { return m_nScreen; }

	screen_mode GetScreenMode() const;
	IPoint		GetResolution() const;
	
	bool		SetScreenMode( screen_mode* psMode );
	bool		SetResoulution( int nWidth, int nHeight );
	bool		SetColorSpace( color_space eColorSpace );
	bool		SetRefreshRate( float vRefreshRate );

/*----- Private or reserved -----------------------------------------*/
private:
	int			m_nCookie;
	screen_id	m_nScreen;
	screen_mode* m_psScreenMode;
	uint32		unused[6];
};


/*----------------------------------------------------------------*/
/*----- inline definitions ---------------------------------------*/


/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _SCREEN_H */
