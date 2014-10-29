//	File name:		Screen.cpp

//	Description:	BScreen let you retrieve and change the display settings.

//	Author:			Stefano Ceccherini (burton666@libero.it)


#include <stdio.h>
#include <OS.h>

// System includes
#include <Screen.h>

#include <ServerProtocol.h>
#include <InterfaceDefs.h>
#include <Application.h>
#include <Message.h>


/*!	\brief Creates a BScreen object which represents the display with the given screen_id
	\param id The screen_id of the screen to get.

	In the current implementation, there is only one display (B_MAIN_SCREEN_ID).
	To be sure that the object was correctly constructed, call IsValid().
*/
BScreen::BScreen(screen_id id)
{
	m_psScreenMode = new screen_mode;

	BMessage cDesktopParams;

	be_app->LockDesktop( id, &cDesktopParams );

	int32  nColorSpace;

	IPoint cResolution;
	int32 screenID;

	cDesktopParams.FindInt32( "desktop", &screenID );
	m_nScreen.id = screenID;
	cDesktopParams.FindIPoint( "resolution", &cResolution );
	cDesktopParams.FindInt32( "color_space", &nColorSpace );
	cDesktopParams.FindFloat( "refresh_rate", &m_psScreenMode->m_vRefreshRate );
	cDesktopParams.FindFloat( "h_pos", &m_psScreenMode->m_vHPos );
	cDesktopParams.FindFloat( "v_pos", &m_psScreenMode->m_vVPos );
	cDesktopParams.FindFloat( "h_size", &m_psScreenMode->m_vHSize );
	cDesktopParams.FindFloat( "v_size", &m_psScreenMode->m_vVSize );

	m_psScreenMode->m_nWidth  = cResolution.x;
	m_psScreenMode->m_nHeight = cResolution.y;
	m_psScreenMode->m_eColorSpace = (color_space)nColorSpace;
}


/*!	\brief Creates a BScreen object which represents the display which contains
	the given BWindow.
	\param win A BWindow.
*/
BScreen::BScreen(BWindow *win)
{
	m_psScreenMode = new screen_mode;

	BMessage cDesktopParams;

	be_app->LockDesktop( B_MAIN_SCREEN_ID, &cDesktopParams );

	int32  nColorSpace;

	IPoint cResolution;
	int32 screenID;

	cDesktopParams.FindInt32( "desktop", &screenID );
	m_nScreen.id = screenID;
	cDesktopParams.FindIPoint( "resolution", &cResolution );
	cDesktopParams.FindInt32( "color_space", &nColorSpace );
	cDesktopParams.FindFloat( "refresh_rate", &m_psScreenMode->m_vRefreshRate );
	cDesktopParams.FindFloat( "h_pos", &m_psScreenMode->m_vHPos );
	cDesktopParams.FindFloat( "v_pos", &m_psScreenMode->m_vVPos );
	cDesktopParams.FindFloat( "h_size", &m_psScreenMode->m_vHSize );
	cDesktopParams.FindFloat( "v_size", &m_psScreenMode->m_vVSize );

	m_psScreenMode->m_nWidth  = cResolution.x;
	m_psScreenMode->m_nHeight = cResolution.y;
	m_psScreenMode->m_eColorSpace = (color_space)nColorSpace;
}

/*!	\brief Releases the resources allocated by the constructor.
*/ 
BScreen::~BScreen()
{
	delete m_psScreenMode;
}


/*!	\brief Returns the color space of the screen display.
	\return \c B_CMAP8, \c B_RGB15, or \c B_RGB32, or \c B_NO_COLOR_SPACE
		if the screen object is invalid.
*/
color_space
BScreen::ColorSpace()
{
	return( m_psScreenMode->m_eColorSpace );
}

/*!	\brief Returns the rectangle that locates the screen in the screen coordinate system.
	\return a BRect that locates the screen in the screen coordinate system.
*/
BRect
BScreen::Frame()
{
	return( BRect( 0.0f, 0.0f, m_psScreenMode->m_nWidth, m_psScreenMode->m_nHeight ) );
}


screen_mode BScreen::GetScreenMode() const
{
	return( *m_psScreenMode );
}


IPoint BScreen::GetResolution() const
{
	return( IPoint( m_psScreenMode->m_nWidth, m_psScreenMode->m_nHeight ) );
}


bool BScreen::SetScreenMode( screen_mode* psMode )
{
	m_psScreenMode->m_nWidth = psMode->m_nWidth;
	m_psScreenMode->m_nHeight = psMode->m_nHeight;
	m_psScreenMode->m_eColorSpace = psMode->m_eColorSpace;
	m_psScreenMode->m_vRefreshRate = psMode->m_vRefreshRate;
	m_psScreenMode->m_vHPos  = psMode->m_vHPos;
	m_psScreenMode->m_vVPos  = psMode->m_vVPos;
	m_psScreenMode->m_vHSize = psMode->m_vHSize;
	m_psScreenMode->m_vVSize = psMode->m_vVSize;
	be_app->SetScreenMode( m_nScreen, SCRMF_RES | SCRMF_COLORSPACE | SCRMF_REFRESH | SCRMF_POS | SCRMF_SIZE,
											m_psScreenMode );
	return( true );
}


bool BScreen::SetResoulution( int nWidth, int nHeight )
{
	m_psScreenMode->m_nWidth = nWidth;
	m_psScreenMode->m_nHeight = nHeight;

	be_app->SetScreenMode( m_nScreen, SCRMF_RES, m_psScreenMode );
	return( true );
}


bool BScreen::SetColorSpace( color_space eColorSpace )
{
	m_psScreenMode->m_eColorSpace = eColorSpace;
	be_app->SetScreenMode( m_nScreen, SCRMF_COLORSPACE, m_psScreenMode );
	return( true );
}


bool BScreen::SetRefreshRate( float vRefreshRate )
{
	m_psScreenMode->m_vRefreshRate = vRefreshRate;
	be_app->SetScreenMode( m_nScreen, SCRMF_REFRESH, m_psScreenMode );
	return( true );
}
