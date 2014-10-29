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

 
#include <Requesters.h>
#include <Screen.h>
#include <Font.h>
#include <Invoker.h>



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ProgressRequester::ProgressRequester( BRect inFrame,
                                      const char* pzTitle, bool bCanSkip ) :
    BWindow( inFrame, pzTitle, B_UNTYPED_WINDOW, 0 )
{
	Lock();
	m_bDoCancel = false;
	m_bDoSkip   = false;

	m_pcProgView = new ProgressView( Bounds(), bCanSkip );
	AddChild( m_pcProgView );
	Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressRequester::MessageReceived( BMessage* pcMessage )
{
	switch( pcMessage->what )
	{
		case IDC_CANCEL:
			m_bDoCancel = true;
			break;

		case IDC_SKIP:
			m_bDoSkip = true;
			break;

		default:
			BHandler::MessageReceived( pcMessage );
			break;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ProgressRequester::DoCancel() const
{
	return( m_bDoCancel );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ProgressRequester::DoSkip()
{
	bool bSkip = m_bDoSkip;
	m_bDoSkip = false;
	return( bSkip );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressRequester::SetPathName( const char* pzString )
{
	m_pcProgView->m_pcPathName->SetText( pzString );
	m_bDoSkip   = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressRequester::SetFileName( const char* pzString )
{
	m_pcProgView->m_pcFileName->SetText( pzString );
	m_bDoSkip   = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ProgressView::ProgressView( BRect inFrame, bool bCanSkip ) : BView( inFrame, "progress_view", B_FOLLOW_ALL, B_WILL_DRAW )
{
	m_pcFileName = new BStringView( BRect(0,0,1,1), "file_name", "" );
	m_pcPathName = new BStringView( BRect(0,0,1,1), "path_name", "" );
	m_pcCancel   = new BButton( BRect(0,0,1,1), "cancel", "Cancel", new BMessage( ProgressRequester::IDC_CANCEL ) );
	if ( bCanSkip )
	{
		m_pcSkip = new BButton( BRect(0,0,1,1), "skip", "Skip", new BMessage( ProgressRequester::IDC_SKIP ) );
	}
	else
	{
		m_pcSkip = NULL;
	}

	AddChild( m_pcPathName );
	AddChild( m_pcFileName );
	AddChild( m_pcCancel );

	if ( m_pcSkip != NULL )
	{
		AddChild( m_pcSkip );
	}

	m_pcPathName->SetLowColor( 220, 220, 220 );
	m_pcPathName->SetHighColor( 0, 0, 0 );

	m_pcFileName->SetLowColor( 220, 220, 220 );
	m_pcFileName->SetHighColor( 0, 0, 0 );

	Layout( Bounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressView::Draw( BRect cUpdateRect )
{
	SetHighColor( 220, 220, 220 );
	FillRect( cUpdateRect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressView::FrameResized( float inWidth, float inHeight )
{
	Layout( Bounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ProgressView::Layout( const BRect& cBounds )
{
	BPoint cSize1;
	BPoint cSize2;

	m_pcCancel->GetPreferredSize( &cSize1.x, &cSize1.y );

	if ( m_pcSkip != NULL )
	{
		 m_pcSkip->GetPreferredSize( &cSize2.x, &cSize2.y );
	}
	else
	{
		cSize2 = cSize1;
	}

	BPoint cStrSize;
	m_pcFileName->GetPreferredSize( &cStrSize.x, &cStrSize.y );

	cStrSize.x = cBounds.Width() - 9.0f;

	if ( cSize2.x > cSize1.x )
	{
		cSize1.x = cSize2.x;
	}

	if ( cSize2.y > cSize1.y )
	{
		cSize1.y = cSize2.y;
	}

	if ( m_pcSkip != NULL )
	{
		m_pcSkip->ResizeTo( cSize1 );
	}

	m_pcCancel->ResizeTo( cSize1 );

	BPoint cPos1 = cBounds.RightBottom() - cSize1 - BPoint( 5, 5 );
	BPoint cPos2 = cPos1;
	cPos2.x -= cSize1.x + 5;

	float nCenter = ((cBounds.Height()+1.0f) - cSize1.y - 5.0f) / 2.0f;

	m_pcPathName->ResizeTo( cStrSize );
	m_pcPathName->MoveTo( 5, nCenter - cStrSize.y / 2 - cStrSize.y );

	m_pcFileName->ResizeTo( cStrSize );
	m_pcFileName->MoveTo( 5, nCenter - cStrSize.y / 2 + cStrSize.y );

	if ( m_pcSkip != NULL )
	{
		m_pcSkip->MoveTo( cPos1  );
		m_pcCancel->MoveTo( cPos2 );
	}
	else
	{
		m_pcCancel->MoveTo( cPos1 );
	}
	Flush();
}
