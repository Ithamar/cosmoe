#include <stdio.h>
#include <float.h>

#include "fontpanel.h"

#include <TextView.h>
#include <Button.h>
#include <DropDownList.h>
#include <ListView.h>
#include <Spinner.h>

#include <Application.h>
#include <Message.h>




enum
{
    ID_APPLY,
    ID_RESTORE,
    ID_RESCAN,
    ID_SEL_CHANGED,
    ID_SELECTED,
    ID_SIZE_CHANGED,
    ID_BM_SIZE_CHANGED,
    ID_CONFIG_NAME_CHANGED
};


class TestView : public BView
{
public:
  TestView( const BRect& cFrame, const char* pzName, uint32 nFollowFlags );
  virtual void Draw( BRect cUpdateRect );
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

TestView::TestView( const BRect& cFrame, const char* pzName, uint32 nFollowFlags ) : BView( cFrame, pzName, nFollowFlags, B_WILL_DRAW )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TestView::Draw( BRect cUpdateRect )
{
  SetHighColor( 255, 255 ,255 );
  FillRect( cUpdateRect );

  SetHighColor( 0, 0, 0 );

  MovePenTo( 30, 40 );
  DrawString( "Test string" );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

FontPanel::FontPanel( const BRect& cFrame ) : LayoutView( cFrame, "", NULL, B_FOLLOW_NONE )
{
    VLayoutNode* pcRoot     = new VLayoutNode( "root" );
    HLayoutNode* pcTopPane  = new HLayoutNode( "top_pane", 1.0f, pcRoot );

    m_pcRescanBut = new BButton( BRect( 0, 0, 0, 0 ), "rescan_but", "Rescan", new BMessage(ID_RESCAN), B_FOLLOW_NONE );
    m_pcOkBut     = new BButton( BRect( 0, 0, 0, 0 ), "ok_but", "Apply", new BMessage(ID_APPLY), B_FOLLOW_NONE );
    m_pcCancelBut = new BButton( BRect( 0, 0, 0, 0 ), "cancel_but", "Restore", new BMessage(ID_RESTORE), B_FOLLOW_NONE );

    m_pcFontList    = new BListView( BRect( 0, 0, 0, 0 ), "font_list", BListView::F_RENDER_BORDER, B_FOLLOW_NONE );
    m_pcSizeSpinner = new Spinner( BRect( 0, 0, 0, 0 ), "size_spinner", 12.0f, new BMessage( ID_SIZE_CHANGED ), B_FOLLOW_NONE );

    m_pcFontConfigList = new CDropDownList( BRect(0,0,0,0), "font_cfg_list", true, B_FOLLOW_NONE );
    m_pcBitmapSizeList = new CDropDownList( BRect(0,0,0,0), "bitmap_size_list", true, B_FOLLOW_NONE );

    m_pcTestView    = new BTextView( BRect( 0, 0, 0, 0 ), "test_view", "Test string", B_FOLLOW_NONE );

    pcRoot->AddChild( m_pcTestView );

    pcTopPane->AddChild( m_pcFontList );
    VLayoutNode* pcRightPane = new VLayoutNode( "right_pane", 0.0f, pcTopPane );

    m_pcTestView->SetMultiLine( true );
    m_pcFontConfigList->SetReadOnly( true );
    m_pcBitmapSizeList->SetReadOnly( true );

    m_pcSizeSpinner->SetMinValue( 1.0 );
    m_pcSizeSpinner->SetMaxValue( 100.0 );
    m_pcSizeSpinner->SetMinPreferredSize( 5 );
    m_pcSizeSpinner->SetMaxPreferredSize( 5 );

    m_pcFontList->InsertColumn( "Name", 130 );
    m_pcFontList->InsertColumn( "Style", 100 );
    m_pcFontList->InsertColumn( "Spacing", 80 );

    pcRightPane->AddChild( m_pcFontConfigList );
    pcRightPane->AddChild( m_pcSizeSpinner );
    pcRightPane->AddChild( m_pcBitmapSizeList );
    pcRightPane->AddChild( new LayoutSpacer( "spacer", 5.0f ) );
    pcRightPane->AddChild( m_pcRescanBut );
    pcRightPane->AddChild( m_pcOkBut );
    pcRightPane->AddChild( m_pcCancelBut );

    pcRoot->SetBorders( BRect( 10,10,10,10 ), "top_pane", "test_view", NULL );
    pcRoot->SetBorders( BRect( 10,5,10,5 ), "font_cfg_list", "bitmap_size_list", "size_spinner", "rescan_but", "ok_but", "cancel_but", NULL );
    pcRoot->SameWidth( "rescan_but", "ok_but", "cancel_but", NULL );

    SetRoot( pcRoot );

    m_pcFontList->SetSelChangeMsg( new BMessage( ID_SEL_CHANGED ) );
    m_pcFontList->SetInvokeMsg( new BMessage( ID_SELECTED ) );

    std::vector<std::string> cConfigList;

    BFont::GetConfigNames( &cConfigList );

    for ( uint i = 0 ; i < cConfigList.size() ; ++i )
	{
        ConfigNode sNode;
        sNode.m_cName = cConfigList[i];
        sNode.m_bModified = false;
        if ( BFont::GetDefaultFont( cConfigList[i], &sNode.m_sConfig ) != 0 )
		{
            printf( "Error: Failed to get default font %s\n", cConfigList[i].c_str() );
            continue;
        }
        sNode.m_sOrigConfig = sNode.m_sConfig;
        m_cConfigList.push_back( sNode );
        m_pcFontConfigList->AppendItem( cConfigList[i].c_str() );
    }
    m_pcBitmapSizeList->SetSelectionMessage( new BMessage( ID_BM_SIZE_CHANGED ) );
    m_pcFontConfigList->SetSelectionMessage( new BMessage( ID_CONFIG_NAME_CHANGED ) );
    m_pcFontConfigList->SetSelection( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::AllAttached()
{
    m_pcRescanBut->SetTarget( this );
    m_pcOkBut->SetTarget( this );
    m_pcCancelBut->SetTarget( this );
    m_pcSizeSpinner->SetTarget( this );
    m_pcFontList->SetTarget( this );
    m_pcBitmapSizeList->SetTarget( this );
    m_pcFontConfigList->SetTarget( this );

    UpdateSelectedConfig();

    m_pcTestView->SetLowColor( 255, 255, 255 );
    m_pcTestView->SetViewColor( 255, 255, 255 );
    m_pcTestView->SetHighColor( 0, 0, 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::FrameResized( float inWidth, float inHeight )
{
    LayoutView::FrameResized( inWidth, inHeight );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::KeyDown( const char *bytes, int32 numBytes )
{
  switch( bytes[0] )
  {
    case B_UP_ARROW:
      break;
    case B_DOWN_ARROW:
      break;
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

void FontPanel::Draw( BRect cUpdateRect )
{
	BRect cBounds = Bounds();

	SetHighColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	FillRect( cBounds );
}

void FontPanel::UpdateSelectedConfig()
{
    int nSelection = m_pcFontConfigList->GetSelection();
    if ( nSelection < 0 )
	{
        return;
    }
    m_nCurSelConfig = nSelection;
    m_pcSizeSpinner->SetValue( m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize );

    m_pcFontList->Clear();
    int nFamCount = BFont::GetFamilyCount();

    for ( int i = 0 ; i < nFamCount ; ++i )
	{
        char zFamily[B_FONT_FAMILY_LENGTH+1];
        BFont::GetFamilyInfo( i, zFamily );

        int nStyleCount = BFont::GetStyleCount( zFamily );
    
        for ( int j = 0 ; j < nStyleCount ; ++j ) {
            char zStyle[B_FONT_STYLE_LENGTH+1];
            uint32 nFlags;
      
            BFont::GetStyleInfo( zFamily, j, zStyle, &nFlags );

            if ( m_cConfigList[m_nCurSelConfig].m_cName == DEFAULT_FONT_FIXED ) {
                if ( (nFlags & B_IS_FIXED) == 0 ) {
                    continue;
                }
            }
            BStringItem* pcRow = new BStringItem();
            pcRow->AppendString( zFamily );
            pcRow->AppendString( zStyle );
            pcRow->AppendString( (nFlags & B_IS_FIXED) ? "mono" : "dynamic" );
            m_pcFontList->InsertRow( pcRow );
        }
    }

    
    uint nCount = m_pcFontList->GetRowCount();
    for ( uint j = 0 ; j < nCount ; ++j ) {
        BStringItem* pcRow = dynamic_cast<BStringItem*>( m_pcFontList->GetRow( j ) );

        if ( pcRow->GetString(0) == m_cConfigList[m_nCurSelConfig].m_sConfig.m_cFamily &&
             pcRow->GetString(1) == m_cConfigList[m_nCurSelConfig].m_sConfig.m_cStyle ) {
            m_pcFontList->Select( j );
            m_pcFontList->MakeVisible( j );
            break;
        }
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FontPanel::MessageReceived( BMessage* pcMessage )
{
    switch( pcMessage->what )
    {
        case ID_RESCAN:
        {
            if ( BFont::Rescan() ) {
                UpdateSelectedConfig();
            }
            break;
        }
        case ID_APPLY:
        {
            for ( uint i = 0 ; i < m_cConfigList.size() ; ++i ) {
                if ( m_cConfigList[i].m_bModified ) {
                    BFont::SetDefaultFont( m_cConfigList[i].m_cName, m_cConfigList[i].m_sConfig );
                    m_cConfigList[i].m_bModified = false;
                }
            }
            break;
        }
        case ID_RESTORE:
        {
            for ( uint i = 0 ; i < m_cConfigList.size() ; ++i ) {
                m_cConfigList[i].m_sConfig = m_cConfigList[i].m_sOrigConfig;
                BFont::SetDefaultFont( m_cConfigList[i].m_cName, m_cConfigList[i].m_sConfig );
            }
            UpdateSelectedConfig();
            break;
        }
        case ID_CONFIG_NAME_CHANGED:
        {
            UpdateSelectedConfig();
            break;
        }
        case ID_SEL_CHANGED:
        {
            int nSel = m_pcFontList->GetFirstSelected();
            if ( nSel >= 0 ) {
                BStringItem* pcRow = dynamic_cast<BStringItem*>( m_pcFontList->GetRow( nSel ) );
                if ( pcRow != NULL ) {
                    m_cConfigList[m_nCurSelConfig].m_sConfig.m_cFamily = pcRow->GetString(0);
                    m_cConfigList[m_nCurSelConfig].m_sConfig.m_cStyle  = pcRow->GetString(1);
                    m_cConfigList[m_nCurSelConfig].m_bModified = true;



                    BFont::GetBitmapSizes( pcRow->GetString(0), pcRow->GetString(1), &m_cBitmapSizes );
                    m_pcBitmapSizeList->Clear();

                    float vMinOffset = FLT_MAX;
                    int   nSelection = -1;
                    for ( uint i = 0 ; i < m_cBitmapSizes.size() ; ++i ) {
                        char zName[64];
                        float vOffset = fabs( m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize - m_cBitmapSizes[i] );
                        if ( vOffset < vMinOffset ) {
                            vMinOffset = vOffset;
                            nSelection = i;
                        }
                        sprintf( zName, "%.2f", m_cBitmapSizes[i] );
                        m_pcBitmapSizeList->AppendItem( zName );
                    }
                    if ( m_cBitmapSizes.size() > 0 ) {
                        m_pcBitmapSizeList->SetSelection( nSelection, false );
                    } else {
                        m_pcBitmapSizeList->SetCurrentString( "" );
                    }
//                    BFont* pcFont = new Font( *m_pcTestView->GetEditor()->GetFont() ); // new Font();
                    BFont* pcFont = m_pcTestView->GetEditor()->GetFont();
                    if ( pcFont->SetFamilyAndStyle( pcRow->GetString(0).c_str(), pcRow->GetString(1).c_str() ) != 0 ) {
                        printf( "Failed to set family and style\n" );
                        break;
                    }
                    Sync();
//                    m_pcTestView->SetFont( pcFont );
//                    m_pcTestView->GetEditor()->SetFont( pcFont );
                }
            }
            break;
        }
        case ID_BM_SIZE_CHANGED:
        {
            int nSelection = m_pcBitmapSizeList->GetSelection();
            if ( nSelection < 0 ) {
                break;
            }
            m_pcSizeSpinner->SetValue( m_cBitmapSizes[nSelection] );
            break;
        }
        case ID_SIZE_CHANGED:
        {
            BFont* pcFont = m_pcTestView->GetEditor()->GetFont();

            m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize = m_pcSizeSpinner->Value();
            m_cConfigList[m_nCurSelConfig].m_bModified = true;


            if ( m_cBitmapSizes.size() > 0 ) {
                float vMinOffset = FLT_MAX;
                int   nSelection = -1;
                for ( uint i = 0 ; i < m_cBitmapSizes.size() ; ++i ) {
                    float vOffset = fabs( m_cConfigList[m_nCurSelConfig].m_sConfig.m_vSize - m_cBitmapSizes[i] );
                    if ( vOffset < vMinOffset ) {
                        vMinOffset = vOffset;
                        nSelection = i;
                    }
                }
                m_pcBitmapSizeList->SetSelection( nSelection, false );
            }
            
            if ( pcFont != NULL ) {
                Sync();
                pcFont->SetSize( m_pcSizeSpinner->Value() );
//                m_pcTestView->GetEditor()->SetFont( pcFont );
//                m_pcTestView->GetEditor()->SetFont( pcFont );
//                m_pcTestView->GetEditor()->FontChanged( pcFont );
            }
            break;
        }
        default:
            BView::MessageReceived( pcMessage );
            break;
    }
}
