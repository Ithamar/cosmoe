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


#ifndef __REQUESTERS_H__
#define __REQUESTERS_H__

#include <stdarg.h>

#include <vector>
#include <string>

#include <Window.h>
#include <Bitmap.h>
#include <StringView.h>
#include <Button.h>
#include <Message.h>




class ProgressView : public BView
{
public:
					ProgressView( BRect cFrame, bool bCanSkip );
	void			Layout( const BRect& cBounds );

	virtual void	Draw( BRect cUpdateRect );
	virtual void	FrameResized( float inWidth, float inHeight );

private:
	friend class ProgressRequester;
	BStringView*	m_pcPathName;
	BStringView*	m_pcFileName;
	BButton*		m_pcCancel;
	BButton*		m_pcSkip;
};


class ProgressRequester : public BWindow
{
public:
	enum { IDC_CANCEL = 1, IDC_SKIP };

					ProgressRequester( BRect inFrame,
									   const char* pzTitle,
									   bool bCanSkip );

	virtual void	MessageReceived( BMessage* pcMessage );

	void			SetPathName( const char* pzString );
	void			SetFileName( const char* pzString );

	bool			DoCancel() const;
	bool			DoSkip();

private:
	ProgressView*	m_pcProgView;
	volatile bool	m_bDoCancel;
	volatile bool	m_bDoSkip;
};

#endif // __REQUESTERS_H__
