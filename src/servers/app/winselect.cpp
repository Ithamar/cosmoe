/*
 *  The Cosmoe application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include "winselect.h"
#include "ServerWindow.h"
#include "AppServer.h"
#include "ServerFont.h"
#include <ServerBitmap.h>
#include "config.h"
#include "Desktop.h"

#include <Screen.h>
#include <Messenger.h>

WinSelect::WinSelect() : Layer( BRect( 0, 0, 1, 1), "", g_pcTopView, 0, NULL, NULL )
{
//    FontNode* pcNode = new FontNode( AppServer::GetInstance()->GetPlainFont() );
	FontNode* pcNode = new FontNode();
	const font_properties* psProp = AppserverConfig::GetInstance()->GetFontConfig( DEFAULT_FONT_REGULAR );
	pcNode->SetFamilyAndStyle( psProp->m_cFamily, psProp->m_cStyle );
	pcNode->SetProperties( int(psProp->m_vSize*64.0f), int(psProp->m_vShear*64.0f), int(psProp->m_vRotation*64.0f) );

	SetFont( pcNode );
	pcNode->Release();

	ServerFont* pcFontInst = m_pcFont->GetInstance();

	int nAscender  = 5;
	int nDescender = 2;

	if ( pcFontInst != NULL )
	{
		nAscender  = pcFontInst->GetAscender();
		nDescender = pcFontInst->GetDescender();
	}

	int nHeight = 10;
	int nWidth = 50;

	m_pcOldFocusWindow = GetActiveWindow(true);
	Layer* pcLayer;

	for( pcLayer = g_pcTopView->GetTopChild() ; pcLayer != NULL ; pcLayer = pcLayer->GetLowerSibling() )
	{
		if ( pcLayer->IsBackdrop() || pcLayer->IsVisible() == false )
		{
			continue;
		}
		ServerWindow* pcWindow = pcLayer->GetWindow();

		if ( pcWindow == NULL )
		{
			continue;
		}

		m_cWindows.push_back( pcWindow );
		nHeight += nAscender + (-nDescender) + 2;

		const char* pzStr = pcWindow->Title();
		if ( pzStr[0] == '\0' )
		{
			pzStr = "*unnamed*";
		}
		ServerFont* pcFontInst = m_pcFont->GetInstance();

		int nStrWidth = 0;

		if ( pcFontInst != NULL )
		{
			nStrWidth = pcFontInst->GetStringWidth( pzStr, strlen( pzStr ) );
		}

		if ( nStrWidth > nWidth )
		{
			nWidth = nStrWidth;
		}
	}
	m_nCurSelect = (m_cWindows.size() > 1) ? 1 : 0;

	if ( m_cWindows.size() > 0 )
	{
		m_cWindows[m_nCurSelect]->SetFocus( true );
	}

	int32 nDesktop = B_MAIN_SCREEN_ID.id;
	display_mode sMode;
	get_desktop_config( &nDesktop, &sMode, NULL );

	BRect r(0, 0, nWidth + 10, nHeight);
	r.OffsetBy(sMode.virtual_width / 2 - nWidth / 2, sMode.virtual_width / 2 - nHeight / 2);
	SetFrame( r );
}


WinSelect::~WinSelect()
{
	g_pcTopView->RemoveChild( this );
}


void WinSelect::UpdateWinList( bool bMoveToFront, bool bSetFocus )
{
	if ( m_cWindows.size() == 0 )
	{
		return;
	}

	if ( m_nCurSelect != 0 || m_cWindows.size() == 1 )
	{
		ServerWindow* pcWindow = m_cWindows[m_nCurSelect];
		// We search for the window to assert it's not been closed,
		// or moved to another desktop since we built the list.
		if ( bMoveToFront )
		{
			for( Layer* pcLayer = g_pcTopView->GetTopChild() ; NULL != pcLayer ; pcLayer = pcLayer->GetLowerSibling() )
			{
				if ( pcWindow == pcLayer->GetWindow() )
				{
					display_mode dm;
					
					g_pcTopView->RemoveChild( pcWindow->GetTopView() );
					g_pcTopView->AddChild( pcWindow->GetTopView(), true );

					desktop->GetDisplayDriver()->GetMode(&dm);
					// Move the window inside screen boundaries if necesarry.
					int screenWidth = dm.virtual_width;
					int screenHeight = dm.virtual_height;

					if ( pcWindow->Frame( false ).Contains( BRect( 0, 0, screenWidth - 1,
																		screenHeight - 1 ) ) == false )
					{
						BRect cFrame = pcWindow->Frame();
						cFrame.OffsetTo( screenWidth / 2 - (cFrame.Width()+1.0f) / 2,
										 screenHeight / 2 - (cFrame.Height()+1.0f) / 2 );

						pcWindow->SetFrame( cFrame );

						if ( pcWindow->GetAppTarget() != NULL )
						{
							BMessage cMsg( M_WINDOW_FRAME_CHANGED );
							cMsg.AddRect( "_new_frame", cFrame );
							if ( pcWindow->GetAppTarget()->SendMessage( &cMsg ) < 0 )
							{
								printf( "WinSelect::UpdateWinList() failed to send M_WINDOW_FRAME_CHANGED to %s\n", pcWindow->Title() );
							}
						}
					}
					break;
				}
			}
		}
	}
	
	if ( bSetFocus == false && m_pcOldFocusWindow != NULL )
	{
		for( Layer* pcLayer = g_pcTopView->GetTopChild() ; NULL != pcLayer ; pcLayer = pcLayer->GetLowerSibling() )
		{
			if ( m_pcOldFocusWindow == pcLayer->GetWindow() )
			{
				m_pcOldFocusWindow->SetFocus( true );
				break;
			}
		}
	}
}


void WinSelect::RequestDraw( const IRect& cUpdateRect, bool bUpdate )
{
	if ( bUpdate )
	{
		BeginUpdate();
	}

	ServerFont* pcFontInst = m_pcFont->GetInstance();

	int nAscender  = 5;
	int nDescender = 2;

	if ( pcFontInst != NULL )
	{
		nAscender  = pcFontInst->GetAscender();
		nDescender = pcFontInst->GetDescender();
	}

	BRect cRect = Bounds();
	SetHighColor( 170, 170, 170, 0 );
	FillRect( cRect );

	BRect cOBounds = Bounds();
	BRect cIBounds = cOBounds;

	cIBounds.left += 1;
	cIBounds.right -= 1;
	cIBounds.top += 1;
	cIBounds.bottom -= 1;

	DrawFrame( cOBounds, FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
	DrawFrame( cIBounds, FRAME_RECESSED | FRAME_THIN | FRAME_TRANSPARENT );

	cRect.left = 4;
	cRect.right = cRect.left + nAscender + (-nDescender) + 2;

	for ( uint i = 0 ; i < m_cWindows.size() ; ++i )
	{
		int nItem = (m_nCurSelect + i) % m_cWindows.size();
		ServerWindow* pcWindow = m_cWindows[nItem];

		SetHighColor( 0, 0, 0, 0 );

		MovePenTo( cRect.left + 5, cRect.top + 5 + nAscender );

		const char* pzStr = pcWindow->Title();
		if ( pzStr[0] == '\0' )
		{
			pzStr = "*unnamed*";
		}
		DrawString( pzStr, -1 );

		cRect.OffsetBy( 0, nAscender + (-nDescender) + 2 );
	}

	if ( bUpdate )
	{
		EndUpdate();
	}
}


void WinSelect::Step( bool bForward )
{
	if ( m_cWindows.size() == 0 )
	{
		return;
	}

	if ( bForward )
	{
		m_nCurSelect = (m_nCurSelect + 1) % m_cWindows.size();
	}
	else
	{
		m_nCurSelect = (m_nCurSelect + m_cWindows.size() - 1) % m_cWindows.size();
	}

	RequestDraw( Bounds(), false );

	if ( m_cWindows.size() > 0 )
	{
		m_cWindows[m_nCurSelect]->SetFocus( true );
	}
}

