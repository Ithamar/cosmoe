#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>

#include <Window.h>
#include <Menu.h>

#include <Application.h>
#include <Message.h>

#include <kernel.h>



#include "fontpanel.h"
#include "screenpanel.h"
#include "windowpanel.h"
#include "keyboardpanel.h"

#include <TabView.h>


class MyWindow : public BWindow
{
public:
    MyWindow( const BRect& cFrame );
    ~MyWindow();

    virtual void        MessageReceived( BMessage* pcMessage );
      // From Window:
    virtual bool        OkToQuit();
private:
    void SetupMenus();
    BMenu* m_pcMenu;
    BTabView* m_pcView;
};


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MyWindow::SetupMenus()
{
	BRect cMenuFrame = Bounds();
	BRect cMainFrame = cMenuFrame;
	cMenuFrame.bottom = 16;

	m_pcMenu = new BMenu( cMenuFrame, "Menu",
						B_FOLLOW_LEFT | B_FOLLOW_RIGHT | B_FOLLOW_TOP,
						B_FULL_UPDATE_ON_RESIZE,
						B_ITEMS_IN_ROW );

	BMenu* pcItem1 = new BMenu( BRect( 0, 0, 100, 20 ), "File", B_ITEMS_IN_COLUMN, B_FOLLOW_LEFT | B_FOLLOW_TOP );

	pcItem1->AddItem( "Quit", new BMessage( B_QUIT_REQUESTED ) );

	m_pcMenu->AddItem( pcItem1 );

	//cMenuFrame.bottom = m_pcMenu->GetPreferredSize( false ).y - 1;
	m_pcMenu->GetPreferredSize( NULL, &cMenuFrame.bottom );
	cMenuFrame.bottom -= 1.0f;
	cMainFrame.top = cMenuFrame.bottom + 1;

	m_pcMenu->SetFrame( cMenuFrame );

	m_pcMenu->SetTargetForItems( this );

	AddChild( m_pcMenu );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyWindow::MyWindow( const BRect& cFrame ) :
    BWindow( cFrame, "Preferences", B_UNTYPED_WINDOW, 0 )
{
    SetupMenus();

    BRect cMainFrame = Bounds();
    BRect cMenuFrame = m_pcMenu->Frame();

    cMainFrame.top = cMenuFrame.bottom + 1.0f;

    cMainFrame.InsetBy( 5, 5 );
  
    m_pcView = new BTabView( cMainFrame, "main_tab" );
    BView* pcFontPanel = new FontPanel( cMainFrame );
    BView* pcScreenPanel = new ScreenPanel( cMainFrame );
    BView* pcWindowPanel = new WindowPanel( cMainFrame );
    BView* pcKeyboardPanel = new KeyboardPanel( cMainFrame );

    m_pcView->AddTab( pcKeyboardPanel );
    m_pcView->AddTab( pcFontPanel );
    m_pcView->AddTab( pcScreenPanel );
    m_pcView->AddTab( pcWindowPanel );
  
    AddChild( m_pcView );
    Activate( true );
    pcFontPanel->MakeFocus( true );

	float width, height;

	m_pcView->GetPreferredSize(&width, &height);
	width  += 10.0f;
	height += cMenuFrame.bottom + 11.0f;
    //Point cMinSize = m_pcView->GetPreferredSize(false) + Point( 10.0f, cMenuFrame.bottom + 11.0f );
    //Point cMaxSize = m_pcView->GetPreferredSize(true);
    ResizeTo( width, height );
    SetSizeLimits( width, height, width, height );

    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome )
    {
        FILE*        hFile;
        char        zPath[ PATH_MAX ];
        struct stat sStat;
        BMessage        cArchive;
        
        strcpy( zPath, pzHome );
        strcat( zPath, "/config/guiprefs.cfg" );

        if ( stat( zPath, &sStat ) >= 0 ) {
            hFile = fopen( zPath, "rb" );
            if ( NULL != hFile ) {
                uint8* pBuffer = new uint8[sStat.st_size];
                fread( pBuffer, sStat.st_size, 1, hFile );
                fclose( hFile );
                cArchive.Unflatten( pBuffer );
                
                BRect cFrame;
                if ( cArchive.FindRect( "window_frame", &cFrame ) == 0 ) {
                    SetFrame( cFrame );
                }
            }
        }
    }
  
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MyWindow::~MyWindow()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MyWindow::MessageReceived( BMessage* pcMessage )
{
  BView*        pcSource;

  if ( pcMessage->FindPointer( "source", (void**) &pcSource ) != 0 ) {
    BWindow::MessageReceived( pcMessage );
    return;
  }
  
  switch( pcMessage->what )
  {
    case MID_ABOUT:      printf( "About...\n" );        break;
    case MID_HELP:       printf( "Help...\n" );                break;
    case MID_LOAD:       printf( "Load...\n" );                break;
    case MID_INSERT:     printf( "Insert...\n" );        break;
    case MID_SAVE:       printf( "Save\n" );                break;
                                 
    case MID_SAVE_AS:    printf( "Save as...\n" );        break;
    case MID_QUIT:       printf( "Quit...\n" );                break;
                                 
    case MID_DUP_SEL:    printf( "Duplicate\n" );        break;
    case MID_DELETE_SEL: printf( "Delete\n" );                break;
                                 
    case MID_RESTORE:    printf( "Restore...\n" );        break;
    case MID_SNAPSHOT:   printf( "Snapshot\n" );        break;
    case MID_RESET:      printf( "Reset...\n" );        break;
    default:
      BWindow::MessageReceived( pcMessage );
  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MyWindow::OkToQuit()
{
    char* pzHome = getenv( "HOME" );
    if ( NULL != pzHome )
    {
        FILE*        hFile;
        char        zPath[ PATH_MAX ];
        BMessage        cArchive;
        
        strcpy( zPath, pzHome );
        strcat( zPath, "/config/guiprefs.cfg" );

        hFile = fopen( zPath, "wb" );

        if ( NULL != hFile ) {
            cArchive.AddRect( "window_frame", Frame() );

            int nSize = cArchive.FlattenedSize();
            uint8* pBuffer = new uint8[nSize];
            cArchive.Flatten( pBuffer, nSize );
            fwrite( pBuffer, nSize, 1, hFile );
            fclose( hFile );
        }
    }
    be_app->PostMessage( B_QUIT_REQUESTED );
    return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int main()
{
	BApplication* pcApp = new BApplication( "application/x-vnd.KHS-font_prefs" );
	MyWindow* pcWindow = new MyWindow( BRect( 100, 50, 450 - 1, 350 - 1 ) );
	pcWindow->Show();
	pcApp->Run();
}
