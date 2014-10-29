#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>

#include <pwd.h>

#include <Window.h>
#include <Button.h>
#include <Bitmap.h>
#include <Sprite.h>
#include <Icon.h>
#include <Alert.h>
#include <DirectoryView.h>

#include <Application.h>
#include <Message.h>

#include <Node.h>

#include "iconview.h"
#include <macros.h>

BBitmap* load_jpeg( const char* pzPath );



class BitmapView;

static BBitmap* g_pcBackDrop = NULL;

class DirWindow : public BWindow
{
public:
	DirWindow( const BRect& cFrame, const char* pzPath );
	virtual void        MessageReceived( BMessage* pcMessage );
private:
	enum { ID_PATH_CHANGED = 1 };
	DirectoryView* m_pcDirView;
};

class DirIconWindow : public BWindow
{
public:
	DirIconWindow( const BRect& cFrame, const char* pzPath, BBitmap* pcBackdrop );
	virtual void        MessageReceived( BMessage* pcMessage );
private:
	enum { ID_PATH_CHANGED = 1 };
	IconView* m_pcDirView;
};

class BitmapView : public BView
{
public:
	BitmapView( BRect cFrame, BBitmap* pcBitmap );
	~BitmapView();

	Icon*               FindIcon( const BPoint& cPos );
	void                Erase( const BRect& cFrame );

	virtual void        MouseDown( BPoint cPosition );
	virtual void        MouseUp( BPoint cPosition );
	virtual void        MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData );
	virtual void        Draw( BRect cUpdateRect );
private:
	BPoint              m_cLastPos;
	BPoint              m_cDragStartPos;
	BRect               m_cSelRect;
	bigtime_t           m_nHitTime;
	BBitmap*            m_pcBitmap;
	std::vector<Icon*>  m_cIcons;
	bool                m_bCanDrag;
	bool                m_bSelRectActive;
};



void Icon::Select( BitmapView* pcView, bool bSelected )
{
	if ( m_bSelected == bSelected )
	{
		return;
	}
	m_bSelected = bSelected;

	pcView->Erase( GetFrame( pcView->GetFont() ) );

	Draw( pcView, BPoint(0,0), true, true );
}


BitmapView::BitmapView( BRect cFrame, BBitmap* pcBitmap ) :
    BView( cFrame, "_bitmap_view", B_FOLLOW_ALL, B_WILL_DRAW )
{
	m_pcBitmap = pcBitmap;

	struct stat sStat;

	m_cIcons.push_back( new Icon( "Root (List)", "icons/root.icon", sStat ) );
	m_cIcons.push_back( new Icon( "Root (Icon)", "icons/root.icon", sStat ) );
	m_cIcons.push_back( new Icon( "Terminal", "icons/terminal.icon", sStat ) );
	//m_cIcons.push_back( new Icon( "Prefs", "icons/prefs.icon", sStat ) );
	m_cIcons.push_back( new Icon( "Pulse", "icons/cpumon.icon", sStat ) );
	m_cIcons.push_back( new Icon( "Calculator", "icons/cpumon.icon", sStat ) );
	//m_cIcons.push_back( new Icon( "Editor", "icons/cpumon.icon", sStat ) );
	m_cIcons.push_back( new Icon( "Guido", "icons/cpumon.icon", sStat ) );
	//m_cIcons.push_back( new Icon( "CPU usage", "icons/cpumon.icon", sStat ) );
	//m_cIcons.push_back( new Icon( "Memory usage", "icons/memmon.icon", sStat ) );

	m_bCanDrag = false;
	m_bSelRectActive = false;

	BPoint cPos( 20, 20 );

	for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
	{
		m_cIcons[i]->m_cPosition.x = cPos.x + 16;
		m_cIcons[i]->m_cPosition.y = cPos.y;

		cPos.y += 50;
		if ( cPos.y > 500 )
		{
			cPos.y = 20;
			cPos.x += 50;
		}
	}
	m_nHitTime = 0;
}


BitmapView::~BitmapView()
{
	delete m_pcBitmap;
}


void BitmapView::Draw( BRect cUpdateRect )
{
	SetDrawingMode( B_OP_COPY );

	BFont* pcFont = GetFont();

	Erase( cUpdateRect );

	for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
	{
		if ( m_cIcons[i]->GetFrame( pcFont ).Contains( cUpdateRect ) )
		{
			m_cIcons[i]->Draw( this, BPoint(0,0), true, true );
		}
	}
}


void BitmapView::Erase( const BRect& cFrame )
{
	if ( m_pcBitmap != NULL )
	{
		BRect cBmBounds = m_pcBitmap->Bounds();
		int nWidth  = int(cBmBounds.Width()) + 1;
		int nHeight = int(cBmBounds.Height()) + 1;
		SetDrawingMode( B_OP_COPY );
		for ( int nDstY = int(cFrame.top) ; nDstY <= cFrame.bottom ; )
		{
			int y = nDstY % nHeight;
			int nCurHeight = min( int(cFrame.bottom) - nDstY + 1, nHeight - y );

			for ( int nDstX = int(cFrame.left) ; nDstX <= cFrame.right ; )
			{
				int x = nDstX % nWidth;
				int nCurWidth = min( int(cFrame.right) - nDstX + 1, nWidth - x );

				BRect cRect( 0, 0, nCurWidth - 1, nCurHeight - 1 );
				DrawBitmap( m_pcBitmap, cRect.OffsetByCopy( x, y ), cRect.OffsetByCopy( nDstX, nDstY ) );
				nDstX += nCurWidth;
			}
			nDstY += nCurHeight;
		}
	}
	else
	{
		rgb_color color = { 0x00, 0x60, 0x6b, 0};
		SetHighColor(color);
		FillRect(cFrame);
	}
}


Icon* BitmapView::FindIcon( const BPoint& cPos )
{
	BFont* pcFont = GetFont();

	for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
	{
		if ( m_cIcons[i]->GetFrame( pcFont ).Contains( cPos ) )
		{
			return( m_cIcons[i] );
		}
	}
	return( NULL );
}


void BitmapView::MouseDown( BPoint cPosition )
{
	MakeFocus( true );

	Icon* pcIcon = FindIcon( cPosition );

	if ( pcIcon != NULL )
	{
		if (  pcIcon->m_bSelected )
		{
			if ( m_nHitTime + 500000 >= system_time() )
			{
				if ( pcIcon->GetName() == "Root (List)" )
				{
					BWindow*   pcWindow = new DirWindow( BRect( 200, 150, 600, 400 ), "/" );
					pcWindow->Activate();
				}
				else if ( pcIcon->GetName() == "Root (Icon)" )
				{
					BWindow*   pcWindow = new DirIconWindow( BRect( 20, 20, 359, 220 ), "/", g_pcBackDrop );
					pcWindow->Activate();
				}
				else  if ( pcIcon->GetName() == "Terminal" )
				{
					pid_t nPid = fork();
					if ( nPid == 0 )
					{
						set_thread_priority( -1, 0 );
						execlp( "cterm", "cterm", NULL );
						exit( 1 );
					}
				}
				else  if ( pcIcon->GetName() == "Prefs" )
				{
					pid_t nPid = fork();
					if ( nPid == 0 )
					{
						set_thread_priority( -1, 0 );
						execlp( "guiprefs", "guiprefs", NULL );
						exit( 1 );
					}
				}
				else  if ( pcIcon->GetName() == "Pulse" )
				{
					pid_t nPid = fork();
					if ( nPid == 0 )
					{
						set_thread_priority( -1, 0 );
						execlp( "pulse", "pulse", NULL );
						exit( 1 );
					}
				}
				else  if ( pcIcon->GetName() == "Calculator" )
				{
					pid_t nPid = fork();
					if ( nPid == 0 )
					{
						set_thread_priority( -1, 0 );
						execlp( "calc", "calc", NULL );
						exit( 1 );
					}
				}
				else  if ( pcIcon->GetName() == "Editor" )
				{
					pid_t nPid = fork();
					if ( nPid == 0 )
					{
						set_thread_priority( -1, 0 );
						execlp( "aedit", "aedit", NULL );
						exit( 1 );
					}
				}
				else  if ( pcIcon->GetName() == "Guido" )
				{
					pid_t nPid = fork();
					if ( nPid == 0 )
					{
						set_thread_priority( -1, 0 );
						execlp( "guido", "guido", NULL );
						exit( 1 );
					}
				}
			}
			else
			{
				m_bCanDrag = true;
			}
			m_nHitTime = system_time();
			return;
		}
	}

	for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
	{
		m_cIcons[i]->Select( this, false );
	}

	if ( pcIcon != NULL )
	{
		m_bCanDrag = true;
		pcIcon->Select( this, true );
	}
	else
	{
		m_bSelRectActive = true;
		m_cSelRect = BRect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
		SetDrawingMode( B_OP_INVERT );
		DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
	}

	Flush();
	m_cLastPos = cPosition;
	m_nHitTime = system_time();
}


void BitmapView::MouseUp( BPoint cPosition )
{
	BMessage pcData((uint32)0);

	m_bCanDrag = false;

	BFont* pcFont = GetFont();
	if ( m_bSelRectActive )
	{
		SetDrawingMode( B_OP_INVERT );
		DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
		m_bSelRectActive = false;

		if ( m_cSelRect.left > m_cSelRect.right )
		{
			float nTmp = m_cSelRect.left;
			m_cSelRect.left = m_cSelRect.right;
			m_cSelRect.right = nTmp;
		}

		if ( m_cSelRect.top > m_cSelRect.bottom )
		{
			float nTmp = m_cSelRect.top;
			m_cSelRect.top = m_cSelRect.bottom;
			m_cSelRect.bottom = nTmp;
		}

		for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
		{
			m_cIcons[i]->Select( this, m_cSelRect.Contains( m_cIcons[i]->GetFrame( pcFont ) ) );
		}
		Flush();
	}
	else if ( Window()->CurrentMessage()->FindMessage( "_drag_message", &pcData ) == B_OK )
	{
		if (pcData.ReturnAddress() == BMessenger( this ))
		{
			BPoint cHotSpot;
			pcData.FindPoint( "_hot_spot", &cHotSpot );

			for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
			{
				BRect cFrame = m_cIcons[i]->GetFrame( pcFont );
				Erase( cFrame );
			}
			Flush();
			for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
			{
				if ( m_cIcons[i]->m_bSelected )
				{
					m_cIcons[i]->m_cPosition += cPosition - m_cDragStartPos;
				}
				m_cIcons[i]->Draw( this, BPoint(0,0), true, true );
			}
			Flush();
		}
	}
}

void BitmapView::MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData )
{
	int32       nButtons = 0x01;

	Window()->CurrentMessage()->FindInt32("buttons", &nButtons);

	m_cLastPos = cNewPos;

    if ( (nButtons & 0x01) == 0 )
	{
        return;
    }
  
    if ( m_bSelRectActive )
	{
        SetDrawingMode( B_OP_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
        m_cSelRect.right = cNewPos.x;
        m_cSelRect.bottom = cNewPos.y;

        BRect cSelRect = m_cSelRect;
        if ( cSelRect.left > cSelRect.right )
		{
            float nTmp = cSelRect.left;
            cSelRect.left = cSelRect.right;
            cSelRect.right = nTmp;
        }

        if ( cSelRect.top > cSelRect.bottom )
		{
            float nTmp = cSelRect.top;
            cSelRect.top = cSelRect.bottom;
            cSelRect.bottom = nTmp;
        }
        BFont* pcFont = GetFont();
        SetDrawingMode( B_OP_COPY );
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
		{
            m_cIcons[i]->Select( this, cSelRect.Contains( m_cIcons[i]->GetFrame( pcFont ) ) );
        }

        SetDrawingMode( B_OP_INVERT );
        DrawFrame( m_cSelRect, FRAME_TRANSPARENT | FRAME_THIN );
    
        Flush();
        return;
    }
  
    if ( m_bCanDrag )
    {
        Flush();
    
        Icon* pcSelIcon = NULL;

        BRect cSelFrame( 1000000, 1000000, -1000000, -1000000 );
    
        BFont* pcFont = GetFont();
        BMessage cData(1234);
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
            if ( m_cIcons[i]->m_bSelected ) {
                cData.AddString( "file/path", m_cIcons[i]->GetName().c_str() );
                cSelFrame = cSelFrame | m_cIcons[i]->GetFrame( pcFont );
                pcSelIcon = m_cIcons[i];
            }
        }
        if ( pcSelIcon != NULL ) {
            m_cDragStartPos = cNewPos; // + cSelFrame.LeftTop() - cNewPos;

            if ( (cSelFrame.Width()+1.0f) * (cSelFrame.Height()+1.0f) < 12000 )
            {
                BBitmap cDragBitmap( cSelFrame, B_RGB32, true );

                BView* pcView = new BView( cSelFrame.OffsetToCopy(0,0), "", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
                cDragBitmap.AddChild( pcView );

                pcView->SetHighColor( 255, 255, 255, 0 );
                pcView->FillRect( cSelFrame.OffsetToCopy(0,0) );


                for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
                    if ( m_cIcons[i]->m_bSelected ) {
                        m_cIcons[i]->Draw( pcView, -cSelFrame.LeftTop(), true, false );
                    }
                }
                cDragBitmap.Sync();

                uint32* pRaster = (uint32*)cDragBitmap.Bits();

                for ( int y = 0 ; y < cSelFrame.Height() + 1.0f ; ++y ) {
                    for ( int x = 0 ; x < cSelFrame.Width()+1.0f ; ++x ) {
                        if ( pRaster[x + y * int(cSelFrame.Width()+1.0f)] != 0x00ffffff &&
                             (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0xff000000) == 0xff000000 ) {
                            pRaster[x + y * int(cSelFrame.Width()+1.0f)] = (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0x00ffffff) | 0xb0000000;
                        }
                    }
                }
                DragMessage( &cData, &cDragBitmap, cNewPos - cSelFrame.LeftTop() );

            } else {
                DragMessage( &cData, cNewPos - cSelFrame.LeftTop(), cSelFrame.OffsetToCopy(0,0) );
            }
        }
        m_bCanDrag = false;
    }
    Flush();
}



DirWindow::DirWindow( const BRect& cFrame, const char* pzPath ) :
    BWindow( cFrame, "", B_UNTYPED_WINDOW, 0 )
{
    m_pcDirView = new DirectoryView( Bounds(), pzPath, BListView::F_MULTI_SELECT, B_FOLLOW_ALL );
    AddChild( m_pcDirView );
    m_pcDirView->SetDirChangeMsg( new BMessage( ID_PATH_CHANGED ) );
    m_pcDirView->MakeFocus();
    SetTitle( m_pcDirView->GetPath().c_str() );
    Show();
}

void DirWindow::MessageReceived( BMessage* pcMessage )
{
    switch( pcMessage->what )
    {
        case ID_PATH_CHANGED:
            SetTitle( m_pcDirView->GetPath().c_str() );
            break;
        default:
            BWindow::MessageReceived( pcMessage );
            break;
    }
}

DirIconWindow::DirIconWindow( const BRect& cFrame, const char* pzPath, BBitmap* pcBitmap ) :
    BWindow( cFrame, "", B_UNTYPED_WINDOW, 0 )
{
    m_pcDirView = new IconView( Bounds(), pzPath, pcBitmap );
    AddChild( m_pcDirView );
    m_pcDirView->SetDirChangeMsg( new BMessage( ID_PATH_CHANGED ) );
    m_pcDirView->MakeFocus();
    SetTitle( m_pcDirView->GetPath().c_str() );
    Show();
}

void DirIconWindow::MessageReceived( BMessage* pcMessage )
{
    switch( pcMessage->what )
    {
        case ID_PATH_CHANGED:
            SetTitle( m_pcDirView->GetPath().c_str() );
            break;
        default:
            BWindow::MessageReceived( pcMessage );
            break;
    }
}

#if 0
bool get_login( std::string* pcName, std::string* pcPassword );

static void authorize( const char* pzLoginName )
{
dbprintf( "main.cpp: authorize '%s'\n", pzLoginName );
    for (;;)
    {
        std::string cName;
        std::string cPassword;

        if ( pzLoginName != NULL || get_login( &cName, &cPassword ) )
        {
            struct passwd* psPass;

dbprintf( "main.cpp: authorize '%s' / '%s'\n", cName.c_str(), cPassword.c_str() );
            if ( pzLoginName != NULL ) {
                psPass = getpwnam( pzLoginName );
            } else {
                psPass = getpwnam( cName.c_str() );
            }

            if ( psPass != NULL ) {
                const char* pzPassWd = crypt(  cPassword.c_str(), "$1$" );
        
                if ( pzLoginName == NULL && pzPassWd == NULL ) {
                    perror( "crypt()" );
                    pzLoginName = NULL;
                    continue;
                }

                if ( pzLoginName != NULL || strcmp( pzPassWd, psPass->pw_passwd ) == 0 ) {
                    setgid( psPass->pw_gid );
                    setuid( psPass->pw_uid );
                    setenv( "HOME", psPass->pw_dir, true );
                    chdir( psPass->pw_dir );
                    break;
                } else {
                    BAlert* pcAlert = new BAlert( "Login failed",  "Incorrect password", "Sorry" );
                    pcAlert->Go();
                }
            } else {
                BAlert* pcAlert = new BAlert( "Login failed",  "No such user", "Sorry" );
                pcAlert->Go();
            }
        }
        pzLoginName = NULL;
    }
    
}
#endif

int main( int argc, char** argv )
{
	BApplication* pcApp = new BApplication( "application/x-vnd.KHS-desktop_manager" );

	#if 0
	if( getuid() == 0 )
	{
		const char* pzLoginName = NULL;

		if ( argc == 2 )
		{
			pzLoginName = argv[1];
		}
		authorize( pzLoginName );
	}
	#endif
	const char* pzBaseDir = getenv( "COSMOE_SYS" );
	if( pzBaseDir == NULL )
	{
		pzBaseDir = "/cosmoe";
	}
	char* pzPath = new char[ strlen(pzBaseDir) + 80 ];
	strcpy( pzPath, pzBaseDir );
	strcat( pzPath, "/backdrop.jpg" );
	g_pcBackDrop = load_jpeg( pzPath );
	delete[] pzPath;

	BWindow* pcBitmapWindow = new BWindow(  BRect( 0, 0, 1599, 1199 ), "",
											B_NO_BORDER_WINDOW_LOOK,
											B_NORMAL_WINDOW_FEEL,
											WND_BACKMOST,
											B_ALL_WORKSPACES );
	pcBitmapWindow->AddChild( new BitmapView( BRect( 0, 0, 1599, 1199 ), g_pcBackDrop ) );
	pcBitmapWindow->Show();

	pcApp->Run();

	return( 0 );
}
