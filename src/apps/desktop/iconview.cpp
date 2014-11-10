#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <assert.h>

#include "iconview.h"

#include <Bitmap.h>
#include <Icon.h>
#include <Window.h>
#include <Font.h>
#include <Message.h>
#include <Messenger.h>

#include <kernel.h>
#include <macros.h>

BBitmap*   Icon::s_pcBitmap[16] = {NULL,};
int        Icon::s_nCurBitmap = 0;


Icon::Icon( const char* pzTitle, const char* pzPath, const struct stat& sStat ) : m_cTitle( pzTitle )
{
	m_bSelected = false;
	m_sStat     = sStat;
	m_bBoundsValid = false;
	m_bStrWidthValid = false;
	if ( s_pcBitmap[0] == NULL )
	{
		for ( int i = 0 ; i < 16 ; ++i )
		{
			s_pcBitmap[i] = new BBitmap( BRect(0, 0, 31, 31), B_RGB32 );
		}
	}

	IconDir sDir;
	IconHeader sHeader;
	const char* pzBaseDir = getenv( "COSMOE_SYS" );
	if( pzBaseDir == NULL )
	{
		pzBaseDir = "/cosmoe";
	}

	std::string	fullpath(pzBaseDir);

	fullpath +='/';
	fullpath += pzPath;

	FILE* hFile = fopen( fullpath.c_str(), "r" );

	if ( hFile == NULL )
	{
		fprintf( stderr, "Icon::ctor: Failed to open file %s\n", pzPath );
		return;
	}

	if ( fread( &sDir, sizeof( sDir ), 1, hFile ) != 1 )
	{
		printf( "Failed to read icon dir\n" );
	}

	if ( sDir.nIconMagic != ICON_MAGIC )
	{
		printf( "Files %s is not an icon\n", pzPath );
		return;
	}

	for ( int i = 0 ; i < sDir.nNumImages ; ++i )
	{
		if ( fread( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 )
		{
			printf( "Failed to read icon header\n" );
		}

		if ( sHeader.nWidth == 32 )
		{
			fread( m_anLarge, 32*32*4, 1, hFile );
		}
		else if ( sHeader.nWidth == 16 )
		{
			fread( m_anSmall, 16*16*4, 1, hFile );
		}
	}
	fclose( hFile );
}

Icon::~Icon()
{
}

void Icon::Select( IconView* pcView, bool bSelected )
{
	if ( m_bSelected == bSelected )
	{
		return;
	}
	m_bSelected = bSelected;

	pcView->Erase( GetFrame( pcView->GetFont() ) );

	Draw( pcView, BPoint(0,0), true, true );
}

float Icon::GetStrWidth( BFont* pcFont )
{
    if ( m_bStrWidthValid == false )
	{
        m_nMaxStrLen = pcFont->GetStringLength( m_cTitle.c_str(), 72.0f );
        m_vStringWidth = pcFont->StringWidth( m_cTitle.c_str(), m_nMaxStrLen );
        m_bStrWidthValid = true;
    }
    return( m_vStringWidth );
}

BRect Icon::GetBounds( BFont* pcFont )
{
    if ( m_bBoundsValid == false )
	{
        font_height sHeight;

        pcFont->GetHeight( &sHeight );
  
        float vStrWidth = max_c( 32.0f, GetStrWidth( pcFont ) + 4 );
        m_cBounds = BRect( 0, 0, vStrWidth, 32 + 2 + sHeight.ascent + sHeight.descent );
        m_bBoundsValid = true;
    }
    return( m_cBounds );
}

BRect Icon::GetFrame( BFont* pcFont )
{
    BRect cBounds = GetBounds( pcFont );
	cBounds.OffsetBy(m_cPosition);
	cBounds.OffsetBy(-cBounds.Width() * 0.5f, 0.0f);
    return( cBounds );
}

void Icon::Draw( BView* pcView, const BPoint& cOffset, bool bLarge, bool bBlendText )
{
	BPoint cPosition = m_cPosition + cOffset;
	if ( bLarge )
	{
		BFont* pcFont = pcView->GetFont();
		if ( pcFont == NULL )
			return;

		float vStrWidth = GetStrWidth( pcFont );
		font_height sHeight;

		pcFont->GetHeight( &sHeight );

		float x = cPosition.x - vStrWidth / 2.0f;
		float y = cPosition.y + 32.0f + sHeight.ascent + 2.0f;

		if ( bBlendText )
		{
			pcView->SetDrawingMode( B_OP_OVER );
		}
		else
		{
			pcView->SetDrawingMode( B_OP_BLEND );
			pcView->SetLowColor( 150, 150, 150 );
		}



		if ( m_bSelected && bBlendText )
		{
			pcView->SetHighColor( 20, 20, 190 );
			BRect cRect( x - 2, y - sHeight.ascent,
						x + vStrWidth + 1, y + sHeight.descent );
			pcView->FillRect( cRect );
		}

		// Add black "shadow" behind file name.
		pcView->SetHighColor( 0, 0, 0 );
		pcView->DrawString( m_cTitle.c_str(), m_nMaxStrLen, BPoint(x + 1, y + 1) );

		if ( bBlendText )
		{
			pcView->SetHighColor( 255, 255, 255, 0xff );
		}
		else
		{
			pcView->SetHighColor( 200, 200, 200, 0xff );
		}

		pcView->DrawString( m_cTitle.c_str(), m_nMaxStrLen, BPoint(x, y) );

		pcView->SetDrawingMode( B_OP_BLEND );

		if ( s_nCurBitmap == 16 )
		{
			s_nCurBitmap = 0;
			pcView->Sync();
		}
		memcpy( s_pcBitmap[s_nCurBitmap]->Bits(), m_anLarge, 32*32 * 4 );

		BRect r(0,0,31,31);
		pcView->DrawBitmap( s_pcBitmap[s_nCurBitmap],
							r,
							r.OffsetByCopy(cPosition.x - 16.0f, cPosition.y) );
		s_nCurBitmap++;
	}
}

BBitmap* Icon::GetBitmap()
{
	if ( s_nCurBitmap == 16 )
	{
		s_nCurBitmap = 0;
	}
	memcpy( s_pcBitmap[s_nCurBitmap]->Bits(), m_anLarge, 32*32 * 4 );

	return( s_pcBitmap[s_nCurBitmap++] );
}

IconView::IconView( BRect cFrame, const char* pzPath, BBitmap* pcBitmap ) :
    BView( cFrame, "_bitmap_view", B_FOLLOW_ALL, B_WILL_DRAW ), m_cPath( pzPath )
{
	m_pcBitmap = pcBitmap;
	m_bCanDrag = false;
	m_bSelRectActive = false;
	m_pcDirChangeMsg = NULL;
	m_pcCurReadDirSession = NULL;
	m_nHitTime = 0;
}

IconView::~IconView()
{
	delete m_pcDirChangeMsg;
}

void IconView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetTarget( Window() );
	ReRead();
}

void IconView::SetPath( const std::string& cPath )
{
	m_cPath = cPath.c_str();
}

std::string IconView::GetPath()
{
	return( std::string( m_cPath.Path() ) );
}

void IconView::LayoutIcons()
{
	BRect cFrame = Bounds();

	BPoint cPos( 20, 20 );

	for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
	{
		m_cIcons[i]->m_cPosition.x = cPos.x + 16;
		m_cIcons[i]->m_cPosition.y = cPos.y;

		BRect cIconFrame = m_cIcons[i]->GetFrame( GetFont() );

		cPos.x += cIconFrame.Width() + 1.0f + 16.0f;


		if ( cPos.x > cFrame.right - 50 )
		{
			cPos.x = 20;
			cPos.y += 50;
		}
		m_cIcons[i]->Draw( this, BPoint(0,0), true, true );
	}
	Flush();
}

int32 IconView::ReadDirectory( void* pData )
{
	ReadDirParam* pcParam = (ReadDirParam*) pData;
	IconView* pcView = pcParam->m_pcView;

	BWindow* pcWnd = pcView->Window();

	if ( pcWnd == NULL ) {
		return(0);
	}

	pcWnd->Lock();
	for ( uint i = 0 ; i < pcView->m_cIcons.size() ; ++i ) {
		pcView->Erase( pcView->m_cIcons[i]->GetFrame( pcView->GetFont() ) );
		delete pcView->m_cIcons[i];
	}
	pcView->m_cIcons.clear();

	pcWnd->Unlock();

	DIR* pDir = opendir( pcView->GetPath().c_str() );
	if ( pDir == NULL ) {
		printf( "Error: IconView::ReadDirectory() Failed to open %s\n", pcView->GetPath().c_str() );
		goto error;
	}
	struct dirent* psEnt;

	while( pcParam->m_bCancel == false && (psEnt = readdir( pDir ) ) != NULL )
	{
		struct stat sStat;
		if ( strcmp( psEnt->d_name, "." ) == 0 || strcmp( psEnt->d_name, ".." ) == 0 ) {
			continue;
		}
		BPath cFilePath( pcView->GetPath().c_str() );
		cFilePath.Append( psEnt->d_name );

		stat( cFilePath.Path(), &sStat );

		Icon* pcIcon;

		if ( S_ISDIR( sStat.st_mode ) == false )
		{
			pcIcon = new Icon( psEnt->d_name, "/icons/file.icon", sStat );
		}
		else
		{
			pcIcon = new Icon( psEnt->d_name, "/icons/folder.icon", sStat );
		}

		pcWnd->Lock();
		pcView->m_cIcons.push_back( pcIcon );
		pcWnd->Unlock();
	}

	closedir( pDir );
	error:
	pcWnd->Lock();
	if ( pcView->m_pcCurReadDirSession == pcParam )
	{
		pcView->m_pcCurReadDirSession = NULL;
	}

	pcView->LayoutIcons();

	pcWnd->Unlock();
	delete pcParam;
	return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::ReRead()
{
	if ( m_pcCurReadDirSession != NULL )
	{
		m_pcCurReadDirSession->m_bCancel = true;
	}
	m_pcCurReadDirSession = new ReadDirParam( this );

	thread_id hTread = spawn_thread( ReadDirectory, "read_dir_thread", 0, m_pcCurReadDirSession );
	if ( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::Draw( BRect cUpdateRect )
{
	BFont* pcFont = GetFont();

	Erase( cUpdateRect );

	for ( uint i = 0 ; i < m_cIcons.size() ; ++i ) {
		if ( m_cIcons[i]->GetFrame( pcFont ).Contains( cUpdateRect ) ) {
			m_cIcons[i]->Draw( this, BPoint(0,0), true, true );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::Erase( const BRect& cFrame )
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
			int nCurHeight = min_c( int(cFrame.bottom) - nDstY + 1, nHeight - y );

			for ( int nDstX = int(cFrame.left) ; nDstX <= cFrame.right ; )
			{
				int x = nDstX % nWidth;
				int nCurWidth = min_c( int(cFrame.right) - nDstX + 1, nWidth - x );
				BRect cRect( 0, 0, nCurWidth - 1, nCurHeight - 1 );
				DrawBitmap( m_pcBitmap, cRect.OffsetByCopy( x, y ), cRect.OffsetByCopy( nDstX, nDstY ) );
				nDstX += nCurWidth;
			}
			nDstY += nCurHeight;
		}
	}
	else
	{
		rgb_color color = { 255, 255, 255, 0 };
		FillRect( cFrame, color );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Icon* IconView::FindIcon( const BPoint& cPos )
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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::KeyDown( const char *bytes, int32 numBytes )
{
	char nChar = bytes[0];

	if ( isprint( nChar ) )
	{
	}
	else
	{
		switch( bytes[0] )
		{
	/*
			case B_DELETE:
			{
				std::vector<std::string> cPaths;
				for ( int i = GetFirstSelected() ; i <= GetLastSelected() ; ++i ) {
				if ( IsSelected( i ) ) {
					FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(i));
					if ( pcRow != NULL ) {
					Path cPath = m_cPath;
					cPath.Append( pcRow->m_cName.c_str() );
					cPaths.push_back( cPath.Path() );
					}
				}
				}
				StartFileDelete( cPaths, BMessenger( this ) );
				break;
			}
			*/
			case B_BACKSPACE:
				m_cPath.Append( ".." );
				ReRead();
		//        PopState();
				DirChanged( m_cPath.Path() );
				break;

			case B_FUNCTION_KEY:
			{
				BLooper* pcLooper = Looper();
				assert( pcLooper != NULL );
				BMessage* pcMsg = pcLooper->CurrentMessage();
				assert( pcMsg != NULL );

				int32 nKeyCode;
				pcMsg->FindInt32( "_raw_key", &nKeyCode );

				switch( nKeyCode )
				{
					case 6: // F5
						ReRead();
						break;
				}
				break;
			}

			default:
				BView::KeyDown( bytes, numBytes );
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

void IconView::MouseDown( BPoint cPosition )
{
	MakeFocus( true );

	Icon* pcIcon = FindIcon( cPosition );

	if ( pcIcon != NULL )
	{
		if (  pcIcon->m_bSelected )
		{
			if ( m_nHitTime + 500000 >= system_time() )
			{
				Invoked();
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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::MouseUp( BPoint cPosition )
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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData )
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
        for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
		{
            if ( m_cIcons[i]->m_bSelected )
			{
                cData.AddString( "file/path", m_cIcons[i]->GetName().c_str() );
                cSelFrame = cSelFrame | m_cIcons[i]->GetFrame( pcFont );
                pcSelIcon = m_cIcons[i];
            }
        }

        if ( pcSelIcon != NULL )
		{
            m_cDragStartPos = cNewPos; // + cSelFrame.LeftTop() - cNewPos;

            if ( (cSelFrame.Width()+1.0f) * (cSelFrame.Height()+1.0f) < 12000 )
            {
                BBitmap cDragBitmap( cSelFrame, B_RGB32,true );

                BView* pcView = new BView( cSelFrame.OffsetToCopy(0,0), "", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
                cDragBitmap.AddChild( pcView );

                pcView->SetHighColor( 255, 255, 255, 255 );
                pcView->FillRect( cSelFrame.OffsetToCopy(0,0) );


                for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
				{
                    if ( m_cIcons[i]->m_bSelected )
					{
                        m_cIcons[i]->Draw( pcView, -cSelFrame.LeftTop(), true, false );
                    }
                }

                cDragBitmap.Sync();

                uint32* pRaster = (uint32*)cDragBitmap.Bits();

                for ( int y = 0 ; y < cSelFrame.Height() + 1.0f ; ++y ) {
                    for ( int x = 0 ; x < cSelFrame.Width()+1.0f ; ++x ) {
                        if ( pRaster[x + y * int(cSelFrame.Width()+1.0f)] != 0xffffffff &&
                             (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0xff000000) == 0x00000000 ) {
                            pRaster[x + y * int(cSelFrame.Width()+1.0f)] = (pRaster[x + y * int(cSelFrame.Width()+1.0f)] & 0x00ffffff) | 0x50000000;
                        }
                    }
                }
                DragMessage( &cData, &cDragBitmap, cNewPos - cSelFrame.LeftTop() );
            }
			else
			{
                DragMessage( &cData, cNewPos - cSelFrame.LeftTop(), cSelFrame.OffsetToCopy(0,0) );
            }
        }
        m_bCanDrag = false;
    }
    Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::Invoked()
{
  Icon* pcIcon = NULL;
  
  for ( uint i = 0 ; i < m_cIcons.size() ; ++i )
  {
    if ( m_cIcons[i]->IsSelected() ) {
      pcIcon = m_cIcons[i];
      break;
    }
  }
  if ( pcIcon == NULL ) {
    return;
  }
  if ( S_ISDIR( pcIcon->m_sStat.st_mode ) == false ) {
//    ListView::Invoked( nFirstRow, nLastRow );
    return;
  }
  
//  m_cStack.push( State( this, m_cPath.Path() ) );
  m_cPath.Append( pcIcon->GetName().c_str() );
  
  ReRead();

//  if ( bBack ) {
//    PopState();
//  }
  DirChanged( m_cPath.Path() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::SetDirChangeMsg( BMessage* pcMsg )
{
  delete m_pcDirChangeMsg;
  m_pcDirChangeMsg = pcMsg;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconView::DirChanged( const std::string& cNewPath )
{
  if ( m_pcDirChangeMsg != NULL ) {
    Invoke( m_pcDirChangeMsg );
  }
  
}
