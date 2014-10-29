#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>

#include "keyboardpanel.h"
#include "coloredit.h"

#include <Button.h>
#include <ListView.h>
#include <CheckBox.h>
#include <Spinner.h>
#include <StringView.h>
#include <Screen.h>

#include <Application.h>
#include <Message.h>

#include <private/app/keymap.h>




enum
{
  ID_CLOSE,
  ID_APPLY,
  ID_REFRESH,
  ID_TIMING_CHANGED,
};


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

KeyboardPanel::KeyboardPanel( const BRect& cFrame ) : LayoutView( cFrame, "", NULL, B_FOLLOW_NONE )
{
    int nKeyDelay;
    int nKeyRepeat;

    HLayoutNode* pcRoot = new HLayoutNode( "root" );
    
    be_app->GetKeyboardConfig( NULL, &nKeyDelay, &nKeyRepeat );
    
    m_pcRefreshBut= new BButton( BRect( 0, 0, 0, 0 ), "refresh_but", "Refresh", new BMessage( ID_REFRESH ), B_FOLLOW_NONE );
    m_pcOkBut     = new BButton( BRect( 0, 0, 0, 0 ), "apply_but", "Apply", new BMessage( ID_APPLY ), B_FOLLOW_NONE );
    m_pcCancelBut = new BButton( BRect( 0, 0, 0, 0 ), "close_but", "Close", new BMessage( B_QUIT_REQUESTED ), B_FOLLOW_NONE );

    m_pcKeymapList  = new BListView( BRect(0,0,0,0), "keymap_list", BListView::F_RENDER_BORDER );
    m_pcKeyDelay    = new Spinner( BRect(0,0,0,0), "key_delay", float(nKeyDelay), new BMessage( ID_TIMING_CHANGED ) );
    m_pcKeyRepeat   = new Spinner( BRect(0,0,0,0), "key_repeat", float(nKeyRepeat), new BMessage( ID_TIMING_CHANGED ) );


    m_pcKeyRepeat->SetMinPreferredSize( 4 );
    m_pcKeyRepeat->SetMaxPreferredSize( 4 );
    m_pcKeyDelay->SetMinPreferredSize( 4 );
    m_pcKeyDelay->SetMaxPreferredSize( 4 );

    pcRoot->AddChild( m_pcKeymapList, 1.0f );


    VLayoutNode* pcRightPanel       = new VLayoutNode( "right_panel", 0.0f, pcRoot );
    VLayoutNode* pcTopRightPanel    = new VLayoutNode( "top_left_panel", 1.0f, pcRightPanel );
    new LayoutSpacer( "spacer", 2.0f, pcRightPanel );
    VLayoutNode* pcBottomRightPanel = new VLayoutNode( "bottom_left_panel", 0.0f, pcRightPanel );

    pcBottomRightPanel->SetHAlignment( B_ALIGN_RIGHT );

    pcTopRightPanel->AddChild( new BStringView( BRect(0,0,0,0), "", "Key delay", B_ALIGN_CENTER ) );
    pcTopRightPanel->AddChild( m_pcKeyDelay );
    pcTopRightPanel->AddChild( new LayoutSpacer( "spacer", 0.5f ) );
    pcTopRightPanel->AddChild( new BStringView( BRect(0,0,0,0), "", "Key repeat", B_ALIGN_CENTER ) );
    pcTopRightPanel->AddChild( m_pcKeyRepeat );

    pcBottomRightPanel->AddChild( m_pcOkBut );
    pcBottomRightPanel->AddChild( m_pcRefreshBut );
    pcBottomRightPanel->AddChild( m_pcCancelBut );
    
    
    m_pcKeyDelay->SetFormat( "%.0f" );
    m_pcKeyDelay->SetMinMax( 10, 1000 );
    m_pcKeyDelay->SetStep( 1.0f );
    m_pcKeyDelay->SetScale( 1.0f );
    
    m_pcKeyRepeat->SetFormat( "%.0f" );
    m_pcKeyRepeat->SetMinMax( 10, 1000 );
    m_pcKeyRepeat->SetStep( 1.0f );
    m_pcKeyRepeat->SetScale( 1.0f );
  
    m_pcKeymapList->InsertColumn( "Key mapping", 10000 );

    pcRoot->SetBorders( BRect(10.0f,10.0f,10.0f,10.0f), "keymap_list", "right_panel", NULL );
    pcRoot->SetBorders( BRect(10.0f,5.0f,10.0f,5.0f), "refresh_but", "apply_but", "close_but", NULL );
    pcRoot->SameWidth( "refresh_but", "apply_but", "close_but", NULL );
    pcRoot->SameWidth( "key_delay", "key_repeat", NULL );
    
    SetRoot( pcRoot );
    m_pcKeymapList->SetInvokeMsg( new BMessage( ID_APPLY ) );

    BApplication* pcApp = be_app;
  
    int nModeCount = pcApp->GetScreenModeCount();

    for ( int i = 0 ; i < nModeCount ; ++i ) {
        screen_mode sMode;
  
        if ( pcApp->GetScreenModeInfo( i, &sMode ) == 0 )
        {
            std::map<color_space,ColorSpace>::iterator i = m_cColorSpaces.find( sMode.m_eColorSpace );
            if ( i == m_cColorSpaces.end() ) {
                m_cColorSpaces.insert( std::map<color_space,ColorSpace>::value_type( sMode.m_eColorSpace, ColorSpace( sMode.m_eColorSpace ) ) );
                i = m_cColorSpaces.find( sMode.m_eColorSpace );
            }
            (*i).second.m_cResolutions.push_back( sMode );
        }
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::AllAttached()
{
  m_pcKeymapList->SetTarget( this );
  m_pcOkBut->SetTarget( this );
  m_pcRefreshBut->SetTarget( this );
  m_pcKeyDelay->SetTarget( this );
  m_pcKeyRepeat->SetTarget( this );
  
  UpdateResList();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::FrameResized( float inWidth, float inHeight )
{
    LayoutView::FrameResized( inWidth, inHeight );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::KeyDown( const char *bytes, int32 numBytes )
{
  switch( bytes[0] )
  {
    case B_UP_ARROW:
    case B_DOWN_ARROW:
    case B_SPACE:
      break;
    default:
      BView::KeyDown( bytes, numBytes );
      break;
  }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::Draw( BRect cUpdateRect )
{
  BRect cBounds = Bounds();

  SetHighColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
  FillRect( cBounds );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void KeyboardPanel::UpdateResList()
{
    const char* pzBaseDir = getenv( "COSMOE_SYS" );
    if( pzBaseDir == NULL )
    {
        pzBaseDir = "/cosmoe";
    }
    char* pzPath = new char[ strlen(pzBaseDir) + 80 ];
    strcpy( pzPath, pzBaseDir );
    strcat( pzPath, "/keymaps" );
    DIR* pDir = opendir( pzPath );
    delete[] pzPath;
    if ( pDir == NULL ) {
        return;
    }
    std::string cKeymap;
    be_app->GetKeyboardConfig( &cKeymap, NULL, NULL );

    m_pcKeymapList->Clear();
    dirent* psEntry;

    int i = 0;
    while( (psEntry = readdir( pDir )) != NULL ) {
        if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 ) {
            continue;
        }

        pzPath = new char[ strlen(pzBaseDir) + strlen(psEntry->d_name) + 80 ];
        strcpy( pzPath, pzBaseDir );
        strcat( pzPath, "/keymaps/" );
        strcat( pzPath, psEntry->d_name );
        FILE* hFile = fopen( pzPath, "r" );
        delete[] pzPath;
        if ( hFile == NULL ) {
            continue;
        }
        uint32 nMagic = 0;
        fread( &nMagic, sizeof(nMagic), 1, hFile );
        fclose( hFile );
        if ( nMagic != KEYMAP_MAGIC ) {
            continue;
        }
    
        BStringItem* pcRow = new BStringItem();

        pcRow->AppendString( psEntry->d_name );      
        m_pcKeymapList->InsertRow( pcRow );
        if ( cKeymap == psEntry->d_name ) {
            m_pcKeymapList->Select( i );
        }
        ++i;
    }
    closedir( pDir );
}

void KeyboardPanel::MessageReceived( BMessage* pcMessage )
{
    switch( pcMessage->what )
    {
        case ID_REFRESH:
            UpdateResList();
            break;
        case ID_TIMING_CHANGED:
            be_app->SetKeyboardTimings( int(m_pcKeyDelay->Value()), int(m_pcKeyRepeat->Value()) );
            break;
        case ID_APPLY:
        {
            int nSelection = m_pcKeymapList->GetFirstSelected();
            if ( nSelection < 0 ) {
                break;
            }
            BStringItem* pcRow = static_cast<BStringItem*>(m_pcKeymapList->GetRow( nSelection ));
            be_app->SetKeymap( pcRow->GetString( 0 ).c_str() );
            break;
        }
        default:
            BView::MessageReceived( pcMessage );
            break;
    }
}
