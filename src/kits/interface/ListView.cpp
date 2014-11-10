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


#include <stdio.h>

#include <ListView.h>
#include <Window.h>

#include <ScrollBar.h>
#include <Button.h>
#include <Font.h>
#include <Message.h>
#include <Application.h>
#include <macros.h>

#include <limits.h>
#include <algorithm>




struct RowContentPred { // : public binary_function<BListItem,BListItem, bool> {
    RowContentPred( int nColumn ) { m_nColumn = nColumn; }
    bool operator()(const BListItem* x, const BListItem* y) const { return( x->IsLessThan( y, m_nColumn ) ); }
    int m_nColumn;
};

struct RowPosPred { // : public binary_function<BListItem,BListItem, bool> {
	bool operator()(const BListItem* x, const BListItem* y) const { return( x->m_vYPos < y->m_vYPos ); }
};

/*
  BListView -+
            |
            +-- ListViewHeader -+
                                |
                                +-- ListViewContainer -+
                                                       |
                                                       +-- ListViewCol
                                                       |
                                                       +-- ListViewCol
                                                       |
                                                       +-- ListViewCol


  +-----------------------------------------------------+
  |                      BListView                       |
  |+---------------------------------------------------+|
  ||               |  ListViewHeader      |            ||
  || +-----------------------------------------------+ ||
  || |               ListViewContainer               | ||
  || | +-------------++-------------++-------------+ | ||
  || | | ListViewCol || ListViewCol || ListViewCol | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | ^ ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | - ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | +-------------++-------------++-------------+ | ||
  || +-----------------------------------------------+ ||
  |+------<------------------------------------->------+|
  +-----------------------------------------------------+
 */



class ListViewCol : public BView
{
public:
						ListViewCol( ListViewContainer* pcParent,
									 const BRect& cFrame,
									 const std::string& cTitle );
						~ListViewCol();

	virtual void		Draw( BRect cUpdateRect );
	void				Refresh( const BRect& cUpdateRect );
	
private:
	friend class ListViewHeader;
	friend class ListViewContainer;

	ListViewContainer*	m_pcParent;
	std::string			m_cTitle;
	float				m_vContentWidth;
};


class ListViewHeader : public BView
{
	friend class BListView;
	friend class BListItem;
	friend class ListViewCol;

						ListViewHeader( BListView* pcParent,
										const BRect& cFrame,
										uint32 nModeFlags );
										
	void				DrawButton( const char* pzTitle, const BRect& cFrame, BFont* pcFont, font_height* psFontHeight );
	virtual void        Draw( BRect cUpdateRect );

	virtual void        MouseDown( BPoint cPosition );
	virtual void        MouseUp( BPoint cPosition );
	virtual void        MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData );
	virtual void        FrameResized( float inWidth, float inHeight );
	virtual void        ViewScrolled( const BPoint& cDelta );

	virtual bool        IsFocus( void ) const;

	void                Layout();

	BListView*           m_pcParent;
	ListViewContainer*  m_pcMainView;

	int                 m_nSizeColumn;
	int                 m_nDragColumn;
	BPoint              m_cHitPos;
	float               m_vHeaderHeight;
};


class ListViewContainer : public BView
{
	friend class BListView;
	friend class ListViewHeader;
	friend class BListItem;
	friend class ListViewCol;

	enum { AUTOSCROLL_TIMER = 1 };

	ListViewContainer( BListView* pcListView, BRect inFrame, uint32 nModeFlags );
	int                        InsertColumn( const char* pzTitle, int nWidth, int nPos = -1 );
	int                        InsertRow( int nPos, BListItem* pcRow, bool bUpdate );
	BListItem*          RemoveRow( int nIndex, bool bUpdate );
	void                InvalidateRow( int nRow, uint32 nFlags, bool bImidiate = false );
	void                Clear();
	int                 GetRowIndex( float y ) const;
	void                MakeVisible( int nRow, bool bCenter );
	void                StartScroll( BListView::scroll_direction eDirection, bool bSelect );
	void                StopScroll();

	virtual void        FrameResized( float inWidth, float inHeight );
	virtual void        MouseDown( BPoint cPosition );
	virtual void        MouseUp( BPoint cPosition );
	virtual void        MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData );
	virtual void        Draw( BRect cUpdateRect );
//	virtual void        TimerTick( int nID );
	virtual void        DetachedFromWindow( void );
	virtual bool        IsFocus( void ) const;

	bool                HandleKey(const char* bytes, int32 numBytes);
	void                LayoutColumns();
	ListViewCol*        GetColumn( int nCol ) const { return( m_cCols[m_cColMap[nCol]] ); }


	bool        ClearSelection();
	bool        SelectRange( int nStart, int nEnd, bool bSelect );
	void        InvertRange( int nStart, int nEnd );
	void        ExpandSelect( int nNumRow, bool bInvert, bool bClear );

	std::vector<ListViewCol*> m_cCols;
	std::vector<int>              m_cColMap;
	std::vector<BListItem*> m_cRows;
	BListView*                 m_pcListView;
	uint32                    m_nModeFlags;
	BRect                      m_cSelectRect;
	bool                      m_bIsSelecting;
	bool                      m_bMouseMoved;
	bool                      m_bDragIfMoved;
	int                       m_nBeginSel;
	int                       m_nEndSel;
	bigtime_t                 m_nMouseDownTime;
	int                       m_nLastHitRow;
	int                       m_nFirstSel;
	int                       m_nLastSel;
	float                     m_vVSpacing;
	float                     m_vTotalWidth; // Total with of columns
	float                     m_vContentHeight;
	int                       m_nSortColumn;
	bool                      m_bAutoScrollUp;
	bool                      m_bAutoScrollDown;
	bool                      m_bMousDownSeen;
	bool                      m_bAutoScrollSelects;
};

class DummyRow : public BListItem
{
public:
	DummyRow() {}

	virtual void AttachToView( BView* pcView, int nColumn ) {}
	virtual void SetRect( const BRect& cRect, int nColumn ) {}

	virtual float Width( BView* pcView, int nColumn ) { return( 0.0f ); }
	virtual float Height( BView* pcView ) { return( 0.0f ); }
	virtual void  Draw( const BRect& cFrame, BView* pcView, uint nColumn,
						bool bSelected, bool bHighlighted, bool bHasFocus ) {};
	virtual bool  IsLessThan( const BListItem* pcOther, uint nColumn ) const { return( false ); }
};



BListItem::BListItem()
{
	fHeight         = 0.0f;
	fSelected       = false;
	m_bHighlighted  = false;
	m_bIsSelectable = true;
}

BListItem::~BListItem()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool BListItem::HitTest( BView* pcView, const BRect& cFrame, int nColumn, BPoint cPos )
{
	return( true );
}


void BListItem::SetIsSelectable( bool bSelectable )
{
	m_bIsSelectable = bSelectable;
}

bool BListItem::IsSelectable() const
{
	return( m_bIsSelectable );
}

bool BListItem::IsSelected() const
{
	return( fSelected );
}

bool BListItem::IsHighlighted() const
{
	return( m_bHighlighted );
}

void BStringItem::AttachToView( BView* pcView, int nColumn )
{
	m_cStrings[nColumn].second = pcView->StringWidth( m_cStrings[nColumn].first.c_str() ) + 5.0f;
}

void BStringItem::SetRect( const BRect& cRect, int nColumn )
{
}

void BStringItem::AppendString( const std::string& cString )
{
	m_cStrings.push_back( std::make_pair( cString, 0.0f ) );
}

const std::string& BStringItem::GetString( int nIndex ) const
{
	return( m_cStrings[nIndex].first );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float BStringItem::Width( BView* pcView, int nIndex )
{
	return( m_cStrings[nIndex].second );
//    return( pcView->StringWidth( m_cStrings[nIndex] ) + 5.0f );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float BStringItem::Height( BView* pcView )
{
	font_height sHeight;
	pcView->GetFontHeight( &sHeight );

	return( sHeight.ascent + sHeight.descent );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BStringItem::Draw( const BRect& cFrame, BView* pcView, uint nColumn,
                               bool bSelected, bool bHighlighted, bool bHasFocus )
{
	font_height sHeight;
	pcView->GetFontHeight( &sHeight );

	pcView->SetHighColor( 255, 255, 255 );
	pcView->FillRect( cFrame );

	float vFontHeight = sHeight.ascent + sHeight.descent;
	float vBaseLine = cFrame.top + (cFrame.Height()+1.0f) / 2 - vFontHeight / 2 + sHeight.ascent;
	pcView->MovePenTo( cFrame.left + 3.0f, vBaseLine );

	if ( bHighlighted && nColumn == 0 )
	{
		pcView->SetHighColor( 255, 255, 255 );
		pcView->SetLowColor( 0, 50, 200 );
	}
	else if ( bSelected && nColumn == 0 )
	{
		pcView->SetHighColor( 255, 255, 255 );
		pcView->SetLowColor( 0, 0, 0 );
	}
	else
	{
		pcView->SetLowColor( 255, 255, 255 );
		pcView->SetHighColor( 0, 0, 0 );
	}

	if ( bSelected && nColumn == 0 )
	{
		rgb_color aColor = { 0, 0, 0, 0 };
		BRect cRect = cFrame;
		cRect.right  = cRect.left + pcView->StringWidth( m_cStrings[nColumn].first.c_str() ) +          4;
		cRect.top    = vBaseLine - sHeight.ascent - 1;
		cRect.bottom = vBaseLine + sHeight.descent + 1;
		pcView->FillRect( cRect, aColor );
	}
	pcView->DrawString( m_cStrings[nColumn].first.c_str() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool BStringItem::IsLessThan( const BListItem* pcOther, uint nColumn ) const
{
	const BStringItem* pcRow = dynamic_cast<const BStringItem*>(pcOther);
	if ( NULL == pcRow || nColumn >= m_cStrings.size() || nColumn >= pcRow->m_cStrings.size() )
	{
		return( false );
	}
	return( m_cStrings[nColumn].first < pcRow->m_cStrings[nColumn].first );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewCol::ListViewCol( ListViewContainer* pcParent, const BRect& cFrame, const std::string& cTitle ) :
    BView( cFrame, "_lv_column", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW ), m_cTitle(cTitle)
{
	m_pcParent = pcParent;
	m_vContentWidth = 0.0f;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewCol::~ListViewCol()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewCol::Refresh( const BRect& cUpdateRect )
{
	if ( m_pcParent->m_cRows.empty() )
	{
		SetHighColor( 255, 255, 255 );
		FillRect( cUpdateRect );
		return;
	}
	BRect cBounds = Bounds();

	BListItem* pcLastRow = m_pcParent->m_cRows[m_pcParent->m_cRows.size()-1];
	if ( cUpdateRect.top > pcLastRow->m_vYPos + pcLastRow->fHeight + m_pcParent->m_vVSpacing ) {
		SetHighColor( 255, 255, 255 );
		FillRect( cUpdateRect );
		return;
	}

	int nFirstRow  = m_pcParent->GetRowIndex( cUpdateRect.top );

	if ( nFirstRow < 0 )
	{
		nFirstRow = 0;
	}

	uint nColumn = 0;
	for ( nColumn = 0 ; nColumn < m_pcParent->m_cCols.size() ; ++nColumn )
	{
		if ( m_pcParent->m_cCols[nColumn] == this )
		{
			break;
		}
	}

	std::vector<BListItem*>& cList = m_pcParent->m_cRows;
	bool bHasFocus = m_pcParent->m_pcListView->IsFocus();

	BRect cFrame( cBounds.left, 0.0f, cBounds.right, 0.0f );

	for ( int i = nFirstRow ; i < int(cList.size()) ; ++i )
	{
		if ( cList[i]->m_vYPos > cUpdateRect.bottom )
		{
			break;
		}
		cFrame.top = cList[i]->m_vYPos;
		cFrame.bottom = cFrame.top + cList[i]->fHeight + m_pcParent->m_vVSpacing - 1.0f;

		cList[i]->Draw( cFrame, this, nColumn, cList[i]->fSelected, cList[i]->m_bHighlighted, bHasFocus );
	}
	if ( cFrame.bottom < cUpdateRect.bottom )
	{
		cFrame.top = cFrame.bottom + 1.0f;
		cFrame.bottom = cUpdateRect.bottom;
		SetHighColor( 255, 255, 255 );
		FillRect( cFrame );
	}
}

//----------------------------------------------------------------------------
// Name:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewCol::Draw( BRect cUpdateRect )
{
	if ( cUpdateRect.IsValid() == false )
	{
		return; // FIXME: Workaround for appserver bug. Fix appserver.
	}

	if ( m_pcParent->m_bIsSelecting )
	{
		m_pcParent->SetDrawingMode( B_OP_INVERT );
		m_pcParent->DrawFrame( m_pcParent->m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		m_pcParent->SetDrawingMode( B_OP_COPY );
	}

	Refresh( cUpdateRect );
	if ( m_pcParent->m_bIsSelecting )
	{
		m_pcParent->SetDrawingMode( B_OP_INVERT );
		m_pcParent->DrawFrame( m_pcParent->m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		m_pcParent->SetDrawingMode( B_OP_COPY );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewContainer::ListViewContainer( BListView* pcListView, BRect inFrame, uint32 nModeFlags ) :
    BView( inFrame, "main_view", B_FOLLOW_NONE, B_WILL_DRAW | B_DRAW_ON_CHILDREN )
{
	m_pcListView     = pcListView;
	m_vVSpacing      = 3.0f;
	m_nSortColumn    = 0;
	m_vTotalWidth    = 0.0f;
	m_vContentHeight = 0.0f;
	m_nBeginSel      = -1;
	m_nEndSel        = -1;
	m_nFirstSel      = -1;
	m_nLastSel       = -1;
	m_nMouseDownTime = 0;
	m_nLastHitRow    = -1;
	m_bMousDownSeen  = false;
	m_bDragIfMoved   = false;
	m_bAutoScrollSelects = false;
	m_bIsSelecting       = false;
	m_nModeFlags         = nModeFlags;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListViewContainer::IsFocus( void ) const
{
	if ( BView::IsFocus() )
	{
		return( true );
	}
	
	for ( uint i = 0 ; i < m_cColMap.size() ; ++i )
	{
		if ( m_cCols[m_cColMap[i]]->IsFocus() )
		{
			return( true );
		}
	}
	
	return( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::DetachedFromWindow( void )
{
	StopScroll();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::StartScroll( BListView::scroll_direction eDirection, bool bSelect )
{
	m_bAutoScrollSelects = bSelect;

	if ( eDirection == BListView::SCROLL_DOWN )
	{
		if ( m_bAutoScrollDown == false )
		{
			m_bAutoScrollDown = true;
			if ( m_bAutoScrollUp == false )
			{
				BLooper* pcLooper = Looper();
				if ( pcLooper != NULL )
				{
					//pcLooper->AddTimer( this, AUTOSCROLL_TIMER, 50000, false );
				}
			}
			else
			{
				m_bAutoScrollUp = false;
			}
		}
	}
	else
	{
		if ( m_bAutoScrollUp == false )
		{
			m_bAutoScrollUp = true;
			if ( m_bAutoScrollDown == false )
			{
				BLooper* pcLooper = Looper();
				if ( pcLooper != NULL )
				{
					//pcLooper->AddTimer( this, AUTOSCROLL_TIMER, 50000, false );
				}
			}
			else
			{
				m_bAutoScrollDown = false;
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

void ListViewContainer::StopScroll()
{
	if ( m_bAutoScrollUp || m_bAutoScrollDown )
	{
		m_bAutoScrollUp = m_bAutoScrollDown = false;
		BLooper* pcLooper = Looper();
		if ( pcLooper != NULL )
		{
			//pcLooper->RemoveTimer( this, AUTOSCROLL_TIMER );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MouseDown( BPoint cPosition )
{
	MakeFocus( true );

	if ( m_cRows.empty() )
	{
		return;
	}

	m_bMousDownSeen = true;

	int        nHitCol = -1;
	int nHitRow;
	if ( cPosition.y >= m_vContentHeight )
	{
		nHitRow = m_cRows.size() - 1;
	}
	else
	{
		nHitRow = GetRowIndex( cPosition.y );
		if ( nHitRow < 0 )
		{
			nHitRow = 0;
		}
	}
	bool  bDoubleClick = false;

	bigtime_t nCurTime = system_time();

	if ( nHitRow == m_nLastHitRow && nCurTime - m_nMouseDownTime < 500000 )
	{
		bDoubleClick = true;
	}
	m_nLastHitRow = nHitRow;
	if ( nHitRow < 0 )
	{
		return;
	}

	BListItem* pcHitRow = (nHitRow>=0) ? m_cRows[nHitRow] : NULL;

	m_nMouseDownTime = nCurTime;

	uint32 nQualifiers = modifiers();

	if ( (nQualifiers & B_SHIFT_KEY) && (m_nModeFlags & BListView::F_MULTI_SELECT) )
	{
		m_nEndSel   = m_nBeginSel;
	}
	else
	{
		m_nBeginSel = nHitRow;
		m_nEndSel   = nHitRow;
	}

	if ( bDoubleClick )
	{
		m_pcListView->Invoked( m_nFirstSel, m_nLastSel );
		return;
	}
	if ( pcHitRow != NULL )
	{
		for ( uint i = 0 ; i < m_cColMap.size() ; ++i )
		{
			BRect cFrame = GetColumn(i)->Frame();
			if ( cPosition.x >= cFrame.left && cPosition.x < cFrame.right )
			{
				BRect cFrame = GetColumn(i)->Frame();
				cFrame.top    = pcHitRow->m_vYPos;
				cFrame.bottom = cFrame.top + pcHitRow->fHeight + m_vVSpacing;
				if ( pcHitRow->HitTest( GetColumn(i), cFrame, i, cPosition ) )
				{
					nHitCol = i;
				}
				break;
			}
		}
	}
	m_bMouseMoved = false;

	if ( nHitCol == 1 && (nQualifiers & B_CONTROL_KEY) == 0 && m_cRows[nHitRow]->fSelected ) {
		m_bDragIfMoved = true;
	}
	else
	{
		ExpandSelect( nHitRow, (nQualifiers & B_CONTROL_KEY),
					((nQualifiers & (B_CONTROL_KEY | B_SHIFT_KEY)) == 0 || (m_nModeFlags & BListView::F_MULTI_SELECT) == 0) );
		if ( m_nModeFlags & BListView::F_MULTI_SELECT )
		{
			m_bIsSelecting = true;
			m_cSelectRect = BRect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
			SetDrawingMode( B_OP_INVERT );
			DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
			SetDrawingMode( B_OP_COPY );
		}
	}
	Flush();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MouseUp( BPoint cPosition )
{
	m_bMousDownSeen = false;

	BLooper* pcLooper = Looper();
	if ( pcLooper != NULL )
	{
		//pcLooper->RemoveTimer( this, AUTOSCROLL_TIMER );
	}

	if ( m_bDragIfMoved )
	{
		ExpandSelect( m_nEndSel, false, true );
		m_bDragIfMoved = false;
	}

	if ( m_bIsSelecting )
	{
		SetDrawingMode( B_OP_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( B_OP_COPY );
		m_bIsSelecting = false;
		Flush();
	}

	// We may have drag data for one of our parents
	
	BMessage cData((uint32)0);
	
	if ( Window()->CurrentMessage()->FindMessage( "_drag_message", &cData ) == B_OK )
	{
		BView::MouseUp( cPosition );
		return;
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData )
{
	m_bMouseMoved = true;
	if ( pcData != NULL )
	{
		BView::MouseMoved( cNewPos, nCode, pcData );
		return;
	}

	if ( m_bMousDownSeen == false )
	{
		return;
	}

	if ( m_bDragIfMoved )
	{
		m_bDragIfMoved = false;
		if ( m_pcListView->DragSelection( m_pcListView->ConvertFromScreen( ConvertToScreen( cNewPos ) ) ) ) {
			m_bMousDownSeen = false;
			if ( m_bIsSelecting )
			{
				SetDrawingMode( B_OP_INVERT );
				DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
				SetDrawingMode( B_OP_COPY );
				m_bIsSelecting = false;
			}
			return;
		}
	}

	if ( m_bIsSelecting )
	{
		SetDrawingMode( B_OP_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( B_OP_COPY );
		m_cSelectRect.right = cNewPos.x;
		m_cSelectRect.bottom = cNewPos.y;

		int nHitRow;
		if ( cNewPos.y >= m_vContentHeight )
		{
			nHitRow = m_cRows.size() - 1;
		}
		else
		{
			nHitRow = GetRowIndex( cNewPos.y );
			if ( nHitRow < 0 )
			{
				nHitRow = 0;
			}
		}

		if ( nHitRow != m_nEndSel )
		{
			ExpandSelect( nHitRow, true, false );
		}
		SetDrawingMode( B_OP_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( B_OP_COPY );
		Flush();
		BRect cBounds = Bounds();

		if ( cNewPos.y < cBounds.top + BListView::AUTOSCROLL_BORDER )
		{
			StartScroll( BListView::SCROLL_DOWN, true );
		}
		else if ( cNewPos.y > cBounds.bottom - BListView::AUTOSCROLL_BORDER )
		{
			StartScroll( BListView::SCROLL_UP, true );
		}
		else
		{
			StopScroll();
		}
	}
}


int ListViewContainer::GetRowIndex( float y ) const
{
	if ( y < 0.0f || m_cRows.empty() )
	{
		return( -1 );
	}

	DummyRow cDummy;
	cDummy.m_vYPos = y;

	std::vector<BListItem*>::const_iterator cIterator = std::lower_bound( m_cRows.begin(),
																			m_cRows.end(),
																			&cDummy,
																			RowPosPred() );

	int nIndex = cIterator - m_cRows.begin() - 1;
	if ( nIndex >= 0 && y > m_cRows[nIndex]->m_vYPos + m_cRows[nIndex]->fHeight + m_vVSpacing )
	{
		nIndex = -1;
	}
	return( nIndex );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::FrameResized( float inWidth, float inHeight )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

#if 0
void ListViewContainer::TimerTick( int nID )
{
	if ( nID != AUTOSCROLL_TIMER )
	{
		BView::TimerTick( nID );
		return;
	}

	BRect cBounds = Bounds();

	if ( cBounds.Height() >= m_vContentHeight )
	{
		return;
	}
	BPoint cMousePos;
	GetMouse( &cMousePos, NULL );

	float vPrevScroll = GetScrollOffset().y;
	float vCurScroll = vPrevScroll;

	if ( m_bAutoScrollDown )
	{
		float nScrollStep = (cBounds.top + BListView::AUTOSCROLL_BORDER) - cMousePos.y;
		nScrollStep /= 2.0f;
		nScrollStep++;

		vCurScroll += nScrollStep;

		if ( vCurScroll > 0 )
		{
			vCurScroll = 0;
		}
	}
	else if ( m_bAutoScrollUp )
	{
		float vMaxScroll = -(m_vContentHeight - Bounds().Height());

		float vScrollStep = cMousePos.y - (cBounds.bottom - BListView::AUTOSCROLL_BORDER);
		vScrollStep *= 0.5f;
		vScrollStep = vScrollStep + 1.0f;

		vCurScroll  -= vScrollStep;
		cMousePos.y += vScrollStep;

		if ( vCurScroll < vMaxScroll )
		{
			vCurScroll = vMaxScroll;
		}
	}
	if ( vCurScroll != vPrevScroll )
	{
		if ( m_bIsSelecting )
		{
			SetDrawingMode( B_OP_INVERT );
			DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
			SetDrawingMode( B_OP_COPY );
		}
		ScrollTo( 0, vCurScroll );
		if ( m_bIsSelecting ) {
			m_cSelectRect.right  = cMousePos.x;
			m_cSelectRect.bottom = cMousePos.y;

			int nHitRow;
			if ( cMousePos.y >= m_vContentHeight ) {
				nHitRow = m_cRows.size() - 1;
			} else {
				nHitRow = GetRowIndex( cMousePos.y );
				if ( nHitRow < 0 ) {
					nHitRow = 0;
				}
			}

			if ( nHitRow != m_nEndSel ) {
				ExpandSelect( nHitRow, true, false );
			}
			SetDrawingMode( B_OP_INVERT );
			DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
			SetDrawingMode( B_OP_COPY );
		}
		Flush();
	}
}
#endif

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListViewContainer::HandleKey(const char* bytes, int32 numBytes)
{
	uint32 nQualifiers = modifiers();
	char nChar = bytes[0];

	if ( m_cRows.empty() )
	{
		return( false );
	}
	
	switch( nChar )
	{
		case 0:
			return( false );
		case B_PAGE_UP:
		case B_UP_ARROW:
		{
			int nPageHeight = 0;

			if ( m_nEndSel == 0 ) {
				return( true );
			}
			if ( m_cRows.empty() == false ) {
				nPageHeight = int(Bounds().Height() / (m_vContentHeight / float(m_cRows.size())));
			}

			if ( m_nEndSel == -1 ) {
				m_nEndSel = m_nBeginSel;
			}
			if ( m_nBeginSel == -1 ) {
				m_nBeginSel = m_nEndSel = m_cRows.size() - 1;
				ExpandSelect( m_nEndSel, false, (nQualifiers & B_SHIFT_KEY) == 0 || (m_nModeFlags & BListView::F_MULTI_SELECT) == 0 );
			} else {
				if ( (nQualifiers & B_SHIFT_KEY) == 0 || (m_nModeFlags & BListView::F_MULTI_SELECT) == 0 ) {
					m_nBeginSel = -1;
				}
				ExpandSelect( m_nEndSel - ((nChar == B_UP_ARROW) ? 1 : nPageHeight),
							false, (nQualifiers & B_SHIFT_KEY) == 0 || (m_nModeFlags & BListView::F_MULTI_SELECT) == 0 );
			}
			if ( (nQualifiers & B_SHIFT_KEY) == 0 ) {
				m_nBeginSel = m_nEndSel;
			}
			if ( m_cRows[m_nEndSel]->m_vYPos < Bounds().top ) {
				MakeVisible( m_nEndSel, false );
			}
			Flush();
			return( true );
		}
		case B_PAGE_DOWN:
		case B_DOWN_ARROW:
		{
			int nPageHeight = 0;

			if ( m_nEndSel == int(m_cRows.size()) - 1 ) {
				return( true );
			}

			if ( m_cRows.size() > 0 ) {
				nPageHeight = int(Bounds().Height() / (m_vContentHeight / float(m_cRows.size())));
			}

			if ( m_nEndSel == -1 ) {
				m_nEndSel = m_nBeginSel;
			}
			if ( m_nBeginSel == -1 ) {
				m_nBeginSel = m_nEndSel =0;
				ExpandSelect( m_nEndSel, false, (nQualifiers & B_SHIFT_KEY) == 0 || (m_nModeFlags & BListView::F_MULTI_SELECT) == 0 );
			} else {
				if ( (nQualifiers & B_SHIFT_KEY) == 0 || (m_nModeFlags & BListView::F_MULTI_SELECT) == 0 ) {
					m_nBeginSel = -1;
				}
				ExpandSelect( m_nEndSel + ((nChar == B_DOWN_ARROW) ? 1 : nPageHeight),
							false, (nQualifiers & B_SHIFT_KEY) == 0 || (m_nModeFlags & BListView::F_MULTI_SELECT) == 0 );
			}
			if ( (nQualifiers & B_SHIFT_KEY) == 0 )
			{
				m_nBeginSel = m_nEndSel;
			}
			if ( m_cRows[m_nEndSel]->m_vYPos + m_cRows[m_nEndSel]->fHeight + m_vVSpacing > Bounds().bottom ) {
				MakeVisible( m_nEndSel, false );
			}
			Flush();
			return( true );
		}
		case '\n':
			if ( m_nFirstSel >= 0 )
			{
				m_pcListView->Invoked( m_nFirstSel, m_nLastSel );
			}
			return( true );
		default:
			return( false );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MakeVisible( int nRow, bool bCenter )
{
	float        vTotRowHeight = m_cRows[nRow]->fHeight + m_vVSpacing;
	float        y = m_cRows[nRow]->m_vYPos;
	BRect        cBounds = Bounds();
	float        vViewHeight = cBounds.Height() + 1.0f;

	if ( m_vContentHeight <= vViewHeight )
	{
		ScrollTo( 0, 0 );
	}
	else
	{
		if ( bCenter )
		{
			float vOffset = y - vViewHeight * 0.5f + vTotRowHeight * 0.5f;
			if ( vOffset < 0.0f )
			{
				vOffset = 0.0f;
			}
			else if ( vOffset >= m_vContentHeight - 1 )
			{
				vOffset = m_vContentHeight - 1;
			}
			
			if ( vOffset < 0.0f )
			{
				vOffset = 0.0f;
			}
			else if ( vOffset > m_vContentHeight - vViewHeight )
			{
				vOffset = m_vContentHeight - vViewHeight;
			}
			ScrollTo( 0, -vOffset );
		}
		else
		{
			float vOffset;
			if ( y + vTotRowHeight * 0.5f < cBounds.top + vViewHeight * 0.5f )
			{
				vOffset = y;
			}
			else
			{
				vOffset = -(vViewHeight - (y + vTotRowHeight));
			}
			
			if ( vOffset < 0.0f )
			{
				vOffset = 0.0f;
			}
			else if ( vOffset > m_vContentHeight - vViewHeight )
			{
				vOffset = m_vContentHeight - vViewHeight;
			}
			ScrollTo( 0, -vOffset );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::LayoutColumns()
{
	float x = 0;

	for ( uint i = 0 ; i < m_cColMap.size() ; ++i )
	{
		BRect cFrame = GetColumn(i)->Frame();

		GetColumn(i)->MoveTo( x, 0 );
		if ( i == m_cColMap.size() - 1 )
		{
			x += GetColumn(i)->m_vContentWidth;
		} else {
			x += cFrame.Width() + 1.0f;
		}
	}
	m_vTotalWidth = x;
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ListViewContainer::InsertColumn( const char* pzTitle, int nWidth, int nPos )
{
	ListViewCol* pcCol = new ListViewCol( this, BRect( 0, 0, nWidth - 1, 16000000.0f ), pzTitle );
	AddChild( pcCol );
	int nIndex;
	if ( nPos == -1 )
	{
		nIndex = m_cCols.size();
		m_cCols.push_back( pcCol );
	}
	else
	{
		nIndex = nPos;
		m_cCols.insert( m_cCols.begin() + nPos, pcCol );
	}
	m_cColMap.push_back( nIndex );
	LayoutColumns();
	return( nIndex );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ListViewContainer::InsertRow( int nPos, BListItem* pcRow, bool bUpdate )
{
	int nIndex;

	if ( nPos >= 0 )
	{
		if ( nPos > int(m_cRows.size()) )
		{
			nPos = m_cRows.size();
		}
		m_cRows.insert( m_cRows.begin() + nPos, pcRow );
		nIndex = nPos;
	}
	else
	{
		if ( (m_nModeFlags & BListView::F_NO_AUTO_SORT) == 0 && m_nSortColumn >= 0 ) {
			std::vector<BListItem*>::iterator cIterator;

			cIterator = std::lower_bound( m_cRows.begin(),
										m_cRows.end(),
										pcRow,
										RowContentPred( m_nSortColumn ) );
			cIterator = m_cRows.insert( cIterator, pcRow );
			nIndex = cIterator - m_cRows.begin();
		}
		else
		{
			nIndex = m_cRows.size();
			m_cRows.push_back( pcRow );
		}
	}
	
	for ( uint i = 0 ; i < m_cCols.size() ; ++i )
	{
		pcRow->AttachToView( m_cCols[i], i );
	}
	
	for ( uint i = 0 ; i < m_cCols.size() ; ++i )
	{
		float vWidth = pcRow->Width( m_cCols[i], i );
		if ( vWidth > m_cCols[i]->m_vContentWidth )
		{
			m_cCols[i]->m_vContentWidth = vWidth;
		}
	}
	
	pcRow->fHeight = pcRow->Height( this );

	m_vContentHeight += pcRow->fHeight + m_vVSpacing;

	for ( uint i = nIndex ; i < m_cRows.size() ; ++i )
	{
		float y = 0.0f;
		if ( i > 0 )
		{
			y = m_cRows[i-1]->m_vYPos + m_cRows[i-1]->fHeight + m_vVSpacing;
		}
		m_cRows[i]->m_vYPos = y;
	}
	
	if ( bUpdate )
	{
		BRect cBounds = Bounds();

		if ( pcRow->m_vYPos + pcRow->fHeight + m_vVSpacing <= cBounds.bottom )
		{
			if ( m_nModeFlags & BListView::F_DONT_SCROLL )
			{
				cBounds.top = pcRow->m_vYPos;
				for ( uint i = 0 ; i < m_cCols.size() ; ++i )
				{
					m_cCols[i]->Invalidate( cBounds );
				}
			}
			else
			{
				cBounds.top = pcRow->m_vYPos + pcRow->fHeight + m_vVSpacing;
				cBounds.bottom += pcRow->fHeight + m_vVSpacing;
				for ( uint i = 0 ; i < m_cCols.size() ; ++i )
				{
					m_cCols[i]->CopyBits( cBounds.OffsetByCopy( 0.0f, -pcRow->fHeight - m_vVSpacing ), cBounds );
					m_cCols[i]->Invalidate( BRect( cBounds.left, pcRow->m_vYPos, cBounds.right, cBounds.top ) );
				}
			}
		}
	}
	return( nIndex );
}


BListItem* ListViewContainer::RemoveRow( int nIndex, bool bUpdate )
{
	BListItem* pcRow = m_cRows[nIndex];
	m_cRows.erase( m_cRows.begin() + nIndex );

	m_vContentHeight -= pcRow->fHeight + m_vVSpacing;

	for ( uint i = nIndex ; i < m_cRows.size() ; ++i )
	{
		float y = 0.0f;
		if ( i > 0 ) {
			y = m_cRows[i-1]->m_vYPos + m_cRows[i-1]->fHeight + m_vVSpacing;
		}
		m_cRows[i]->m_vYPos = y;
	}

	for ( uint i = 0 ; i < m_cCols.size() ; ++i )
	{
		m_cCols[i]->m_vContentWidth = 0.0f;
		for ( uint j = 0 ; j < m_cRows.size() ; ++j )
		{
			float vWidth = m_cRows[j]->Width( m_cCols[i], i );
			if ( vWidth > m_cCols[i]->m_vContentWidth )
			{
				m_cCols[i]->m_vContentWidth = vWidth;
			}
		}
	}

	if ( bUpdate )
	{
		BRect cBounds = Bounds();

		float vRowHeight = pcRow->fHeight + m_vVSpacing;
		float y   = pcRow->m_vYPos;

		if ( y + vRowHeight <= cBounds.bottom )
		{
			if ( m_nModeFlags & BListView::F_DONT_SCROLL )
			{
				cBounds.top = y;
				for ( uint i = 0 ; i < m_cCols.size() ; ++i )
				{
					m_cCols[i]->Invalidate( cBounds );
				}
			}
			else
			{
				cBounds.top = y + vRowHeight;
				cBounds.bottom += vRowHeight;
				for ( uint i = 0 ; i < m_cCols.size() ; ++i )
				{
					m_cCols[i]->CopyBits( cBounds, cBounds.OffsetByCopy( 0.0f, -vRowHeight ) );
				}
			}
		}
	}
	
	bool bSelChanged = pcRow->fSelected;
	if ( m_nFirstSel != -1 )
	{
		if ( m_nFirstSel == m_nLastSel && nIndex == m_nFirstSel )
		{
			m_nFirstSel = m_nLastSel = -1;
			bSelChanged = true;
		}
		else
		{
			if ( nIndex < m_nFirstSel )
			{
				m_nFirstSel--;
				m_nLastSel--;
				bSelChanged = true;
			}
			else if ( nIndex == m_nFirstSel )
			{
				m_nLastSel--;
				for ( int i = nIndex ; i <= m_nLastSel ; ++i )
				{
					if ( m_cRows[i]->fSelected )
					{
						m_nFirstSel = i;
						bSelChanged = true;
						break;
					}
				}
			}
			else if ( nIndex < m_nLastSel )
			{
				m_nLastSel--;
				bSelChanged = true;
			}
			else if ( nIndex == m_nLastSel )
			{
				for ( int i = m_nLastSel - 1 ; i >= m_nFirstSel ; --i )
				{
					if ( m_cRows[i]->fSelected )
					{
						m_nLastSel = i;
						bSelChanged = true;
						break;
					}
				}
			}
		}
	}
	
	if ( m_nBeginSel != -1 && m_nBeginSel >= int(m_cRows.size()) )
	{
		m_nBeginSel = m_cRows.size() - 1;
	}
	
	if ( m_nEndSel != -1 && m_nEndSel >= int(m_cRows.size()) )
	{
		m_nEndSel = m_cRows.size() - 1;
	}
	
	if ( bSelChanged )
	{
		m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
	}
	
	return( pcRow );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::Clear()
{
	if ( m_bIsSelecting )
	{
		SetDrawingMode( B_OP_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( B_OP_COPY );
		m_bIsSelecting = false;
		Flush();
	}
	m_bMouseMoved  = true;
	m_bDragIfMoved = false;

	for ( uint i = 0 ; i < m_cRows.size() ; ++i )
	{
		delete m_cRows[i];
	}

	m_cRows.clear();
	m_vContentHeight = 0.0f;

	BRect cBounds = Bounds();
	for ( uint i = 0 ; i < m_cColMap.size() ; ++i )
	{
		GetColumn(i)->Invalidate( cBounds );
	}

	for ( uint i = 0 ; i < m_cCols.size() ; ++i )
	{
		m_cCols[i]->m_vContentWidth = 0.0f;
	}

	m_nBeginSel      = -1;
	m_nEndSel        = -1;
	m_nFirstSel      = -1;
	m_nLastSel       = -1;
	m_nMouseDownTime = 0;
	m_nLastHitRow    = -1;
	ScrollTo( 0, 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::Draw( BRect cUpdateRect )
{
	BRect cFrame = Bounds();
	cFrame.top = 0.0f;

	if ( m_cColMap.size() > 0 )
	{
		BRect cLastCol = GetColumn(m_cColMap.size()-1)->Frame();
		cFrame.left = cLastCol.right + 1.0f;
	}
	SetHighColor( 255, 255, 255 );
	cFrame.bottom = COORD_MAX;
	FillRect( cFrame );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::InvalidateRow( int nRow, uint32 nFlags, bool bImidiate )
{
	float vTotRowHeight = m_cRows[nRow]->fHeight + m_vVSpacing;
	float y = m_cRows[nRow]->m_vYPos;
	BRect cFrame( 0.0f, y, COORD_MAX, y + vTotRowHeight - 1.0f );

	if ( Bounds().Contains( cFrame ) == false )
	{
		return;
	}

	if ( bImidiate )
	{
		bool bHasFocus = m_pcListView->IsFocus();

		for ( uint i = 0 ; i < m_cColMap.size() ; ++i )
		{
			BRect cColFrame( cFrame );
			cColFrame.right = GetColumn(i)->Frame().Width();
			m_cRows[nRow]->Draw( cColFrame, GetColumn(i), i, m_cRows[nRow]->fSelected, m_cRows[nRow]->m_bHighlighted, bHasFocus );
		}
	}
	else
	{
		for ( uint i = 0 ; i < m_cColMap.size() ; ++i )
		{
			BRect cColFrame( cFrame );
			cColFrame.right = GetColumn(i)->Frame().Width();
			GetColumn(i)->Invalidate( cColFrame );
		}
	}
}


void ListViewContainer::InvertRange( int nStart, int nEnd )
{
	for ( int i = nStart ; i <= nEnd ; ++i )
	{
		if ( m_cRows[i]->m_bIsSelectable )
		{
			m_cRows[i]->fSelected = !m_cRows[i]->fSelected;
			InvalidateRow( i, BListView::INV_VISUAL );
		}
	}

	if ( m_nFirstSel == -1 )
	{
		m_nFirstSel = nStart;
		m_nLastSel  = nEnd;
	}
	else
	{
		if ( nEnd < m_nFirstSel )
		{
			for ( int i = nStart ; i <= nEnd ; ++i )
			{
				if ( m_cRows[i]->fSelected )
				{
					m_nFirstSel = i;
					break;
				}
			}
		}
		else if ( nStart > m_nLastSel )
		{
			for ( int i = nEnd ; i >= nStart ; --i )
			{
				if ( m_cRows[i]->fSelected )
				{
					m_nLastSel = i;
					break;
				}
			}
		}
		else
		{
			if ( nStart <= m_nFirstSel )
			{
				m_nFirstSel = m_nLastSel + 1;
				for ( int i = nStart ; i <= m_nLastSel ; ++i )
				{
					if ( m_cRows[i]->fSelected )
					{
						m_nFirstSel = i;
						break;
					}
				}
			}

			if ( nEnd >= m_nLastSel )
			{
				m_nLastSel = m_nFirstSel - 1;
				if ( m_nFirstSel < int(m_cRows.size()) )
				{
					for ( int i = nEnd ; i >= m_nFirstSel ; --i )
					{
						if ( m_cRows[i]->fSelected )
						{
							m_nLastSel = i;
							break;
						}
					}
				}
			}
		}
	}

	if ( m_nLastSel < m_nFirstSel )
	{
		m_nFirstSel = -1;
		m_nLastSel  = -1;
	}
}


bool ListViewContainer::SelectRange( int nStart, int nEnd, bool bSelect )
{
	bool bChanged = false;

	for ( int i = nStart ; i <= nEnd ; ++i )
	{
		if ( m_cRows[i]->m_bIsSelectable && m_cRows[i]->fSelected != bSelect )
		{
			m_cRows[i]->fSelected = bSelect;
			InvalidateRow( i, BListView::INV_VISUAL );
			bChanged = true;
		}
	}

	if ( bChanged )
	{
		if ( m_nFirstSel == -1 )
		{
			if ( bSelect )
			{
				m_nFirstSel = nStart;
				m_nLastSel  = nEnd;
			}
		}
		else
		{
			if ( bSelect )
			{
				if ( nStart < m_nFirstSel )
				{
					m_nFirstSel = nStart;
				}
				if ( nEnd > m_nLastSel )
				{
					m_nLastSel = nEnd;
				}
			}
			else
			{
				if ( nEnd >= m_nFirstSel )
				{
					int i = m_nFirstSel;
					m_nFirstSel = m_nLastSel + 1;
					for ( ; i <= m_nLastSel ; ++i )
					{
						if ( m_cRows[i]->fSelected )
						{
							m_nFirstSel = i;
							break;
						}
					}
				}
				if ( nStart <= m_nLastSel && m_nLastSel < int(m_cRows.size()) )
				{
					int i = m_nLastSel;
					m_nLastSel = m_nFirstSel - 1;
					for ( ; i >= m_nFirstSel - 1 ; --i )
					{
						if ( m_cRows[i]->fSelected )
						{
							m_nLastSel = i;
							break;
						}
					}
				}
			}
			if ( m_nLastSel < m_nFirstSel )
			{
				m_nFirstSel = -1;
				m_nLastSel  = -1;
			}
		}
	}
	return( bChanged );
}


void ListViewContainer::ExpandSelect( int nRow, bool bInvert, bool bClear )
{
	if ( m_cRows.empty() )
	{
		return;
	}

	if ( nRow < 0 )
	{
		nRow = 0;
	}

	if ( nRow >= int(m_cRows.size()) )
	{
		nRow = m_cRows.size() - 1;
	}

	if ( bClear ) {
		if ( m_nFirstSel != -1 ) {
			for ( int i = m_nFirstSel ; i <= m_nLastSel ; ++i ) {
				if ( m_cRows[i]->fSelected ) {
					m_cRows[i]->fSelected = false;
					InvalidateRow( i, BListView::INV_VISUAL, true );
				}
			}
		}
		m_nFirstSel = -1;
		m_nLastSel  = -1;
		if ( m_nBeginSel == -1 ) {
			m_nBeginSel = nRow;
		}
		m_nEndSel   = nRow;
		bool bChanged;
		if ( m_nBeginSel < nRow ) {
			bChanged = SelectRange( m_nBeginSel, nRow, true );
		} else {
			bChanged = SelectRange( nRow, m_nBeginSel, true );
		}
		if ( bChanged ) {
			m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
		}
		return;
	}
	
	if ( m_nBeginSel == -1 || m_nEndSel == -1 )
	{
		m_nBeginSel = nRow;
		m_nEndSel   = nRow;
		bool bSelChanged;
		if ( bInvert )
		{
			InvertRange( nRow, nRow );
			bSelChanged = true;
		}
		else
		{
			bSelChanged = SelectRange( nRow, nRow, true );
		}
		if ( bSelChanged )
		{
			m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
		}
		return;
	}

	if ( nRow == m_nEndSel )
	{
		bool bSelChanged;
		if ( bInvert )
		{
			InvertRange( nRow, nRow );
			bSelChanged = true;
		}
		else
		{
			bSelChanged = SelectRange( nRow, nRow, true );
		}
		if ( bSelChanged )
		{
			m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
		}
		m_nEndSel = nRow;
		return;
	}
	
	bool bSelChanged = false;
	if ( m_nBeginSel <= m_nEndSel )
	{
		if ( nRow < m_nEndSel )
		{
			if ( bInvert )
			{
				if ( nRow < m_nBeginSel )
				{
					InvertRange( m_nBeginSel + 1, m_nEndSel );
					InvertRange( nRow, m_nBeginSel - 1 );
				}
				else
				{
					InvertRange( nRow + 1, m_nEndSel );
				}
				bSelChanged = true;
			}
			else
			{
				if ( nRow < m_nBeginSel )
				{
					bSelChanged = SelectRange( m_nBeginSel + 1, m_nEndSel, false );
					bSelChanged = SelectRange( nRow, m_nBeginSel - 1, true ) || bSelChanged;
				}
				else
				{
					bSelChanged = SelectRange( nRow + 1, m_nEndSel, false );
				}
			}
		}
		else
		{
			if ( bInvert )
			{
				InvertRange( m_nEndSel + 1, nRow );
				bSelChanged = true;
			}
			else
			{
				bSelChanged = SelectRange( m_nEndSel + 1, nRow, true );
			}
		}
	}
	else
	{
		if ( nRow < m_nEndSel )
		{
			if ( bInvert )
			{
				InvertRange( nRow, m_nEndSel - 1 );
				bSelChanged = true;
			}
			else
			{
				bSelChanged = SelectRange( nRow, m_nEndSel - 1, true );
			}
		} else {
			if ( bInvert )
			{
				if ( nRow > m_nBeginSel )
				{
					InvertRange( m_nEndSel, m_nBeginSel - 1 );
					InvertRange( m_nBeginSel + 1, nRow );
				}
				else
				{
					InvertRange( m_nEndSel, nRow - 1 );
				}
				bSelChanged = true;
			}
			else
			{
				if ( nRow > m_nBeginSel )
				{
					bSelChanged = SelectRange( m_nEndSel, m_nBeginSel - 1, false );
					bSelChanged = SelectRange( m_nBeginSel + 1, nRow, true ) || bSelChanged;
				}
				else
				{
					bSelChanged = SelectRange( m_nEndSel, nRow - 1, false );
				}
			}
		}
	}

	if ( bSelChanged )
	{
		m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
	}
	m_nEndSel = nRow;
}


bool ListViewContainer::ClearSelection()
{
	bool bChanged = false;

	if ( m_nFirstSel != -1 ) {
		for ( int i = m_nFirstSel ; i <= m_nLastSel ; ++i )
		{
			if ( m_cRows[i]->fSelected )
			{
				bChanged = true;
				m_cRows[i]->fSelected = false;
				InvalidateRow( i, BListView::INV_VISUAL, true );
			}
		}
	}
	m_nFirstSel = -1;
	m_nLastSel  = -1;
	return( bChanged );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewHeader::ListViewHeader( BListView* pcParent, const BRect& cFrame, uint32 nModeFlags ) :
    BView( cFrame, "header_view", B_FOLLOW_LEFT | B_FOLLOW_TOP, 0 )
{
	m_pcParent = pcParent;
	m_nSizeColumn = -1;
	m_nDragColumn = -1;
	m_pcMainView = new ListViewContainer( pcParent, cFrame, nModeFlags );
	AddChild( m_pcMainView );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListViewHeader::IsFocus( void ) const
{
	if ( BView::IsFocus() )
		return( true );
	
	return( m_pcMainView->IsFocus() );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::DrawButton( const char* pzTitle, const BRect& cFrame, BFont* pcFont, font_height* psFontHeight )
{
	SetViewColor( get_default_color( COL_LISTVIEW_TAB ) );
	DrawFrame( cFrame, FRAME_RAISED );

	SetHighColor( get_default_color( COL_LISTVIEW_TAB_TEXT ) );
	SetLowColor( get_default_color( COL_LISTVIEW_TAB ) );

	float vFontHeight = psFontHeight->ascent + psFontHeight->descent;

	int nStrLen = pcFont->GetStringLength( pzTitle, cFrame.Width() - 9.0f );
	DrawString( pzTitle, nStrLen,
				cFrame.LeftTop() + BPoint( 5, (cFrame.Height()+1.0f) / 2 - vFontHeight / 2 + psFontHeight->ascent ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::Draw( BRect cUpdateRect )
{
	BFont* pcFont = GetFont();
	if ( pcFont == NULL )
	{
		return;
	}

	font_height sHeight;
	pcFont->GetHeight( &sHeight );

	BRect cFrame;
	for ( uint i = 0 ; i < m_pcMainView->m_cColMap.size() ; ++i )
	{
		ListViewCol* pcCol = m_pcMainView->GetColumn( i );
		cFrame = pcCol->Frame();
		cFrame.top = 0;
		cFrame.bottom = sHeight.ascent + sHeight.descent + 6 - 1;
		if ( i == m_pcMainView->m_cColMap.size() - 1 )
		{
			cFrame.right = COORD_MAX;
		}
		DrawButton( pcCol->m_cTitle.c_str(), cFrame, pcFont, &sHeight );
	}/*
	cFrame.left = cFrame.right + 1;
	cFrame.right = Bounds().right;
	if ( cFrame.IsValid() ) {
		DrawButton( "", cFrame, pcFont, &sHeight );
	}*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData )
{
	if ( pcData != NULL )
	{
		BView::MouseMoved( cNewPos, nCode, pcData );
		return;
	}

	if ( m_nDragColumn != -1 )
	{
		if ( m_nDragColumn < int(m_pcMainView->m_cColMap.size()) )
		{
			BMessage cData( (uint32)0 );
			cData.AddInt32( "column", m_pcMainView->m_cColMap[m_nDragColumn] );
			BRect cFrame( m_pcMainView->GetColumn( m_nDragColumn )->Frame() );
//            if ( m_nDragColumn == int(m_pcMainView->m_cColMap.size()) - 1 )
//			  {
//                cFrame.right = cFrame.left + m_pcMainView->GetColumn( m_nDragColumn )->m_vContentWidth;
//            }
			m_pcMainView->ConvertToParent( &cFrame );
			ConvertFromParent( &cFrame );
			cFrame.top = 0.0f;
			cFrame.bottom = m_vHeaderHeight - 1.0f;

			DragMessage( &cData, cNewPos - cFrame.LeftTop() - GetScrollOffset(), cFrame.OffsetToCopy(0,0), this );
		}
		m_nDragColumn = -1;
	}

	if ( m_nSizeColumn == -1 )
	{
		return;
	}
	float vColWidth = m_pcMainView->GetColumn( m_nSizeColumn )->Frame().Width();
	float vDeltaSize = cNewPos.x - m_cHitPos.x;
	if ( vColWidth + vDeltaSize < 4.0f )
	{
		vDeltaSize = 4.0f - vColWidth;
	}
	m_pcMainView->GetColumn( m_nSizeColumn )->ResizeBy( vDeltaSize, 0 );

	ListViewCol* pcCol = m_pcMainView->m_cCols[m_pcMainView->m_cColMap[m_pcMainView->m_cColMap.size() - 1]];
	BRect cFrame = pcCol->Frame();
	cFrame.right = m_pcParent->Bounds().right - GetScrollOffset().x;
	pcCol->SetFrame( cFrame );

	m_pcMainView->LayoutColumns();
	m_pcParent->AdjustScrollBars( false );
	Draw( Bounds() );
	Flush();
	m_cHitPos = cNewPos;
}


void ListViewHeader::ViewScrolled( const BPoint& cDelta )
{
	if ( m_pcMainView->m_cColMap.empty() )
	{
		return;
	}
	ListViewCol* pcCol = m_pcMainView->m_cCols[m_pcMainView->m_cColMap[m_pcMainView->m_cColMap.size() - 1]];
	BRect cFrame = pcCol->Frame();
	cFrame.right = m_pcParent->Bounds().right - GetScrollOffset().x;
	pcCol->SetFrame( cFrame );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::MouseDown( BPoint cPosition )
{
	if ( m_pcMainView->m_cColMap.empty() )
	{
		return;
	}

	if ( cPosition.y >= m_pcMainView->Frame().top )
	{
		return;
	}

	BPoint cMVPos = m_pcMainView->ConvertFromParent( cPosition );
	for ( uint i = 0 ; i < m_pcMainView->m_cColMap.size() ; ++i )
	{
		BRect cFrame( m_pcMainView->GetColumn(i)->Frame() );

		if ( cMVPos.x >= cFrame.left && cMVPos.x <= cFrame.right )
		{
			if ( i > 0 && cMVPos.x >= cFrame.left && cMVPos.x <= cFrame.left + 5 )
			{
				m_nSizeColumn = i - 1;
				m_cHitPos = cPosition;
				MakeFocus( true );
				break;
			}
			if ( cMVPos.x >= cFrame.right - 5 && cMVPos.x <= cFrame.right )
			{
				m_nSizeColumn = i;
				m_cHitPos = cPosition;
				MakeFocus( true );
				break;
			}
			m_nDragColumn = i;
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

void ListViewHeader::MouseUp( BPoint cPosition )
{
	m_nSizeColumn = -1;

	if ( m_nDragColumn != -1 && m_nDragColumn < int(m_pcMainView->m_cColMap.size()) ) {
		m_pcMainView->m_nSortColumn = m_pcMainView->m_cColMap[m_nDragColumn];

		if ( (m_pcMainView->m_nModeFlags & BListView::F_NO_AUTO_SORT) == 0 )
		{
			m_pcParent->Sort();
			BRect cBounds = m_pcMainView->Bounds();
			for ( uint j = 0 ; j < m_pcMainView->m_cColMap.size() ; ++j )
			{
				if ( m_pcMainView->GetColumn(j) != NULL )
				{
					m_pcMainView->GetColumn(j)->Invalidate( cBounds );
				}
			}
			Flush();
		}
	}
	
	m_nDragColumn = -1;

	BMessage pcData((uint32)0);
	
	if ( Window()->CurrentMessage()->FindMessage( "_drag_message", &pcData ) == B_OK )
	{
		int32 nColumn;
		if ( pcData.FindInt32( "column", &nColumn ) == 0 )
		{
			BPoint cMVPos = m_pcMainView->ConvertFromParent( cPosition );
			for ( uint i = 0 ; i < m_pcMainView->m_cColMap.size() ; ++i )
			{
				BRect cFrame( m_pcMainView->GetColumn(i)->Frame() );

				if ( cMVPos.x >= cFrame.left && cMVPos.x <= cFrame.right )
				{
					int j;
					if ( cMVPos.x < cFrame.left + cFrame.Width() * 0.5f )
					{
						j = i;
					}
					else
					{
						j = i + 1;
					}
					
					m_pcMainView->m_cColMap.insert( m_pcMainView->m_cColMap.begin() + j, nColumn );
					for ( int k = 0 ; k < int(m_pcMainView->m_cColMap.size()) ; ++k ) {
						if ( k != j && m_pcMainView->m_cColMap[k] == nColumn )
						{
							m_pcMainView->m_cColMap.erase( m_pcMainView->m_cColMap.begin() + k );
							break;
						}
					}
					m_pcMainView->LayoutColumns();
					Draw( Bounds() );
					Flush();
				}
			}
		}
		else
		{
			BView::MouseUp( cPosition );
		}
		return;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::FrameResized( float inWidth, float inHeight )
{
	BRect cBounds( Bounds() );
	bool  bNeedFlush = false;

	if ( inWidth != 0.0f )
	{
		BRect cDamage = cBounds;

		cDamage.left = cDamage.right - max_c( 2.0f, inWidth + 1.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}

	if ( inHeight != 0.0f )
	{
		BRect cDamage = cBounds;

		cDamage.top = cDamage.bottom - max_c( 2.0f, inHeight + 1.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}

	Layout();

	if ( bNeedFlush )
		Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::Layout()
{
	font_height sHeight;
	GetFontHeight( &sHeight );

	if ( m_pcMainView->m_nModeFlags & BListView::F_NO_HEADER )
	{
		m_vHeaderHeight = 0.0f;
	}
	else
	{
		m_vHeaderHeight = sHeight.ascent + sHeight.descent + 6.0f;
	}

	BRect cFrame = Frame();
	cFrame.OffsetTo(0,0);
	cFrame.Set(floor(cFrame.left), floor(cFrame.top),
				floor(cFrame.right), floor(cFrame.bottom));
	cFrame.top += m_vHeaderHeight;
	cFrame.right = COORD_MAX;
	m_pcMainView->SetFrame( cFrame );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BListView::BListView( BRect inFrame, const char* inTitle, uint32 inModeFlags, uint32 inResizeMask, uint32 inFlags ) :
        BControl( inFrame, inTitle, "", NULL, inResizeMask, inFlags )
{
	m_pcHeaderView   = new ListViewHeader( this, BRect( 0, 0, 1, 1 ), inModeFlags );
	m_pcMainView     = m_pcHeaderView->m_pcMainView;
	m_pcVScroll      = NULL;
	m_pcHScroll      = NULL;
	m_pcSelChangeMsg = NULL;
	m_pcInvokeMsg    = NULL;
//    m_nModeFlags     = inModeFlags;

	AddChild( m_pcHeaderView );
	Layout();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BListView::~BListView()
{
    delete m_pcSelChangeMsg;
    delete m_pcInvokeMsg;
}


void BListView::LabelChanged( const std::string& cNewLabel )
{
}


void BListView::EnableStatusChanged( bool bIsEnabled )
{
}


bool BListView::IsMultiSelect() const
{
    return( m_pcMainView->m_nModeFlags & F_MULTI_SELECT );
}


void BListView::SetMultiSelect( bool bMulti )
{
	if ( bMulti )
	{
		m_pcMainView->m_nModeFlags |= F_MULTI_SELECT;
	}
	else
	{
		m_pcMainView->m_nModeFlags &= ~F_MULTI_SELECT;
	}
}


bool BListView::IsAutoSort() const
{
	return( (m_pcMainView->m_nModeFlags & F_NO_AUTO_SORT) == 0 );
}


void BListView::SetAutoSort( bool bAuto )
{
	if ( bAuto )
	{
		m_pcMainView->m_nModeFlags &= ~F_NO_AUTO_SORT;
	}
	else
	{
		m_pcMainView->m_nModeFlags |= F_NO_AUTO_SORT;
	}
}


bool BListView::HasBorder() const
{
	return( m_pcMainView->m_nModeFlags & F_RENDER_BORDER );
}


void BListView::SetRenderBorder( bool bRender )
{
	if ( bRender )
	{
		m_pcMainView->m_nModeFlags |= F_RENDER_BORDER;
	}
	else
	{
		m_pcMainView->m_nModeFlags &= ~F_RENDER_BORDER;
	}

	Layout();
	Flush();
}


bool BListView::HasColumnHeader() const
{
	return( (m_pcMainView->m_nModeFlags & F_NO_HEADER) == 0 );
}


void BListView::SetHasColumnHeader( bool bFlag )
{
	if ( bFlag )
	{
		m_pcMainView->m_nModeFlags &= ~F_NO_HEADER;
	}
	else
	{
		m_pcMainView->m_nModeFlags |= F_NO_HEADER;
	}

	Layout();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool BListView::IsFocus( void ) const
{
	if ( BView::IsFocus() )
	{
		return( true );
	}

	return( m_pcHeaderView->IsFocus() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::AllAttached()
{
	BView::AllAttached();
	SetTarget( Window() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::KeyDown(const char* bytes, int32 numBytes)
{
	if ( m_pcMainView->HandleKey(bytes, numBytes) == false )
	{
		BView::KeyDown(bytes, numBytes);
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::SetSelChangeMsg( BMessage* pcMsg )
{
	if ( pcMsg != m_pcSelChangeMsg )
	{
		delete m_pcSelChangeMsg;
		m_pcSelChangeMsg = pcMsg;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::SetInvokeMsg( BMessage* pcMsg )
{
	if ( pcMsg != m_pcInvokeMsg )
	{
		delete m_pcInvokeMsg;
		m_pcInvokeMsg = pcMsg;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BMessage* BListView::GetSelChangeMsg() const
{
	return( m_pcSelChangeMsg );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BMessage* BListView::GetInvokeMsg() const
{
	return( m_pcInvokeMsg );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
//        Calculate and set the range and proportion of scroll bars.
//        Scroll the view so upper/left corner
//        is at or above/left of 0,0 if needed
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::AdjustScrollBars( bool bOkToHScroll )
{
	if ( m_pcMainView->m_cRows.size() == 0 )
	{
		if ( m_pcVScroll != NULL )
		{
			RemoveChild( m_pcVScroll );
			delete m_pcVScroll;
		}

		if ( m_pcHScroll != NULL )
		{
			RemoveChild( m_pcHScroll );
			delete m_pcHScroll;
		}

		if ( m_pcVScroll != NULL || m_pcHScroll != NULL )
		{
			m_pcVScroll = NULL;
			m_pcHScroll = NULL;
			m_pcMainView->ScrollTo( 0, 0 );
			m_pcHeaderView->ScrollTo( 0, 0 );
			Layout();
		}
	}
	else
	{
		float vViewHeight    = m_pcMainView->Bounds().Height() + 1.0f;
		float vViewWidth     = m_pcHeaderView->Bounds().Width() + 1.0f;
		float vContentHeight = m_pcMainView->m_vContentHeight;

		float vProportion;
		if ( vContentHeight > 0 && vContentHeight > vViewHeight  )
		{
			vProportion = vViewHeight / vContentHeight;
		}
		else
		{
			vProportion = 1.0f;
		}

		if ( vContentHeight > vViewHeight )
		{
			if ( m_pcVScroll == NULL )
			{
				m_pcVScroll  = new BScrollBar( BRect( -1, -1, 0, 0 ), "v_scroll", NULL, 0, 1000, B_VERTICAL );
				AddChild( m_pcVScroll );
				m_pcVScroll->SetTarget( m_pcMainView );
				Layout();
			}
			else
			{
				m_pcVScroll->SetSteps( ceil( vContentHeight / float(m_pcMainView->m_cRows.size()) ), ceil( vViewHeight * 0.8f ) );
				m_pcVScroll->SetProportion( vProportion );
				m_pcVScroll->SetRange( 0, max_c( vContentHeight - vViewHeight, 0.0f ) );
			}
		}
		else
		{
			if ( m_pcVScroll != NULL )
			{
				RemoveChild( m_pcVScroll );
				delete m_pcVScroll;
				m_pcVScroll = NULL;
				m_pcMainView->ScrollTo( 0, 0 );
				Layout();
			}
		}

		if ( m_pcVScroll != NULL )
		{
			vViewWidth -= m_pcVScroll->Frame().Width();
		}

		if ( m_pcMainView->m_vTotalWidth > 0 && m_pcMainView->m_vTotalWidth > vViewWidth ) {
			vProportion = vViewWidth / m_pcMainView->m_vTotalWidth;
		}
		else
		{
			vProportion = 1.0f;
		}

		if ( m_pcMainView->m_vTotalWidth > vViewWidth )
		{
			if ( m_pcHScroll == NULL )
			{
				m_pcHScroll  = new BScrollBar( BRect( -1, -1, 0, 0 ), "h_scroll", NULL, 0, 1000, B_HORIZONTAL );
				AddChild( m_pcHScroll );
				m_pcHScroll->SetTarget( m_pcHeaderView );
				Layout();
			}
			else
			{
				m_pcHScroll->SetSteps( 15.0f, ceil( vViewWidth * 0.8f ) );
				m_pcHScroll->SetProportion( vProportion );
				m_pcHScroll->SetRange( 0, m_pcMainView->m_vTotalWidth - vViewWidth );
			}
		}
		else
		{
			if ( m_pcHScroll != NULL )
			{
				RemoveChild( m_pcHScroll );
				delete m_pcHScroll;
				m_pcHScroll = NULL;
				m_pcHeaderView->ScrollTo( 0, 0 );
				Layout();
			}
		}

		if ( bOkToHScroll )
		{
			float nOff = m_pcHeaderView->GetScrollOffset().x;
			if ( nOff < 0 )
			{
				vViewWidth  = m_pcHeaderView->Bounds().Width() + 1.0f;
				if ( vViewWidth - nOff > m_pcMainView->m_vTotalWidth )
				{
					float nDeltaScroll = min_c( (vViewWidth - nOff) - m_pcMainView->m_vTotalWidth, -nOff );
					m_pcHeaderView->ScrollBy( nDeltaScroll, 0 );
				}
				else if ( vViewWidth > m_pcMainView->m_vTotalWidth )
				{
					m_pcHeaderView->ScrollBy( -nOff, 0 );
				}
			}
		}

		float nOff = m_pcMainView->GetScrollOffset().y;
		if ( nOff < 0 )
		{
			vViewHeight = m_pcMainView->Bounds().Height() + 1.0f;
			if ( vViewHeight - nOff > vContentHeight )
			{
				float nDeltaScroll = min_c( (vViewHeight - nOff) - vContentHeight, -nOff );
				m_pcMainView->ScrollBy( 0, nDeltaScroll );
			}
			else if ( vViewHeight > vContentHeight )
			{
				m_pcMainView->ScrollBy( 0, -nOff );
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

void BListView::Layout()
{
	font_height sHeight;

	GetFontHeight( &sHeight );

	float nTopHeight = m_pcHeaderView->m_vHeaderHeight;

	BRect cFrame( Bounds() );
	cFrame.OffsetTo(0,0);
	cFrame.Set(cFrame.left, cFrame.top, floor(cFrame.right), floor(cFrame.bottom));
	if ( m_pcMainView->m_nModeFlags & F_RENDER_BORDER )
	{
		cFrame.InsetBy( 2.0f, 2.0f );
	}

	BRect cHeaderFrame( cFrame );

	if ( m_pcHScroll != NULL )
	{
		cHeaderFrame.bottom -= 16.0f;
	}

	if ( m_pcVScroll != NULL )
	{
		BRect cVScrFrame( cFrame );

		cVScrFrame.top    += nTopHeight;
//        cVScrFrame.bottom -= 2.0f;
		if ( m_pcHScroll != NULL ) {
			cVScrFrame.bottom -= B_H_SCROLL_BAR_HEIGHT;
		}
		cVScrFrame.left = cVScrFrame.right - B_V_SCROLL_BAR_WIDTH;

		m_pcVScroll->SetFrame( cVScrFrame );
	}

	if ( m_pcHScroll != NULL )
	{
		BRect cVScrFrame( cFrame );
		cHeaderFrame.bottom = floor( cHeaderFrame.bottom );
		if ( m_pcVScroll != NULL ) {
			cVScrFrame.right -= B_V_SCROLL_BAR_WIDTH;
		}
//        cVScrFrame.left  += 2.0f;
//        cVScrFrame.right -= 2.0f;
		cVScrFrame.top = cVScrFrame.bottom - B_H_SCROLL_BAR_HEIGHT;
		cHeaderFrame.bottom = cVScrFrame.top;

		m_pcHScroll->SetFrame( cVScrFrame );
	}
	m_pcHeaderView->SetFrame( cHeaderFrame );
	AdjustScrollBars();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::FrameResized( float inWidth, float inHeight )
{
	if ( m_pcMainView->m_cColMap.empty() == false )
	{
		ListViewCol* pcCol = m_pcMainView->m_cCols[m_pcMainView->m_cColMap[m_pcMainView->m_cColMap.size() - 1]];
		BRect cFrame = pcCol->Frame();
		cFrame.right = Bounds().right - m_pcHeaderView->GetScrollOffset().x;

		pcCol->SetFrame( cFrame );
	}

	Layout();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Sort()
{
	if ( uint(m_pcMainView->m_nSortColumn) >= m_pcMainView->m_cCols.size() )
	{
		return;
	}

	if ( m_pcMainView->m_cRows.empty() )
	{
		return;
	}
	std::sort( m_pcMainView->m_cRows.begin(),
			m_pcMainView->m_cRows.end(),
			RowContentPred( m_pcMainView->m_nSortColumn ) );

	float y = 0.0f;
	for ( uint i = 0 ; i < m_pcMainView->m_cRows.size() ; ++i )
	{
		m_pcMainView->m_cRows[i]->m_vYPos = y;
		y += m_pcMainView->m_cRows[i]->fHeight + m_pcMainView->m_vVSpacing;
	}

	if ( m_pcMainView->m_nFirstSel != -1 )
	{
		for ( uint i = 0 ; i < m_pcMainView->m_cRows.size() ; ++i )
		{
			if ( m_pcMainView->m_cRows[i]->fSelected )
			{
				m_pcMainView->m_nFirstSel = i;
				break;
			}
		}
		for ( int i = m_pcMainView->m_cRows.size() - 1 ; i >= 0 ; --i )
		{
			if ( m_pcMainView->m_cRows[i]->fSelected ) {
				m_pcMainView->m_nLastSel = i;
				break;
			}
		}
	}

	for ( uint i = 0 ; i < m_pcMainView->m_cCols.size() ; ++i )
	{
		m_pcMainView->m_cCols[i]->Invalidate();
	}
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::MakeVisible( int nRow, bool bCenter )
{
	m_pcMainView->MakeVisible( nRow, bCenter );
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int BListView::InsertColumn( const char* pzTitle, int nWidth, int nPos )
{
	int nColumn = m_pcMainView->InsertColumn( pzTitle, nWidth, nPos );
	Layout();
	Flush();
	return( nColumn );
}

const column_map& BListView::GetColumnMapping() const
{
	return( m_pcMainView->m_cColMap );
}

void BListView::SetColumnMapping( const column_map& cMap )
{
	for ( int i = 0 ; i < int(m_pcMainView->m_cCols.size()) ; ++i )
	{
		if ( std::find( cMap.begin(), cMap.end(), i ) == cMap.end() )
		{
			if ( std::find( m_pcMainView->m_cColMap.begin(), m_pcMainView->m_cColMap.end(), i ) != m_pcMainView->m_cColMap.end() )
			{
				m_pcMainView->m_cCols[i]->Hide();
			}
		}
		else
		{
			if ( std::find( m_pcMainView->m_cColMap.begin(), m_pcMainView->m_cColMap.end(), i ) == m_pcMainView->m_cColMap.end() )
			{
				m_pcMainView->m_cCols[i]->Show();
			}
		}
	}
	m_pcMainView->m_cColMap = cMap;
	m_pcMainView->LayoutColumns();
	m_pcHeaderView->Invalidate();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::InsertRow( int nPos, BListItem* pcRow, bool bUpdate )
{
	m_pcMainView->InsertRow( nPos, pcRow, bUpdate );
	AdjustScrollBars();
	if ( bUpdate )
	{
		Flush();
	}
}

void BListView::InsertRow( BListItem* pcRow, bool bUpdate )
{
	InsertRow( -1, pcRow, bUpdate );
}

BListItem* BListView::RemoveRow( int nIndex, bool bUpdate )
{
	BListItem* pcRow = m_pcMainView->RemoveRow( nIndex, bUpdate );
	AdjustScrollBars();
	if ( bUpdate )
	{
		Flush();
	}
	return( pcRow );
}

void BListView::InvalidateRow( int nRow, uint32 nFlags )
{
	m_pcMainView->InvalidateRow( nRow, nFlags );
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

uint BListView::GetRowCount() const
{
	return( m_pcMainView->m_cRows.size() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BListItem* BListView::GetRow( uint nIndex ) const
{
	if ( nIndex < m_pcMainView->m_cRows.size() )
	{
		return( m_pcMainView->m_cRows[nIndex] );
	}

	return( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BListItem* BListView::GetRow( const BPoint& cPos ) const
{
	int nHitRow = m_pcMainView->GetRowIndex( cPos.y );
	if ( nHitRow >= 0 )
	{
		return( m_pcMainView->m_cRows[nHitRow] );
	}

	return( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int BListView::HitTest( const BPoint& cPos ) const
{
	BPoint cParentPos = ConvertToScreen( m_pcMainView->ConvertFromScreen( cPos ) );
	int nHitRow = m_pcMainView->GetRowIndex( cParentPos.y );
	if ( nHitRow >= 0 )
	{
		return( nHitRow );
	}

	return( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float BListView::GetRowPos( int nRow )
{
	if ( nRow >= 0 && nRow < int(m_pcMainView->m_cRows.size()) )
	{
		return( m_pcMainView->m_cRows[nRow]->m_vYPos + m_pcMainView->Frame().top + m_pcMainView->GetScrollOffset().y );
	}

	return( 0.0f );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Select( int nFirst, int nLast, bool bReplace, bool bSelect )
{
	if ( m_pcMainView->m_cRows.empty() )
	{
		return;
	}

	if ( (bReplace || (m_pcMainView->m_nModeFlags & F_MULTI_SELECT) == 0) && m_pcMainView->m_nFirstSel != -1 )
	{
		for ( int i = m_pcMainView->m_nFirstSel ; i <= m_pcMainView->m_nLastSel ; ++i )
		{
			if ( m_pcMainView->m_cRows[i]->fSelected )
			{
				m_pcMainView->m_cRows[i]->fSelected = false;
				m_pcMainView->InvalidateRow( i, INV_VISUAL );
			}
		}
		m_pcMainView->m_nFirstSel = -1;
		m_pcMainView->m_nLastSel  = -1;
	}

	if ( m_pcMainView->SelectRange( nFirst, nLast, bSelect ) )
	{
		SelectionChanged( m_pcMainView->m_nFirstSel, m_pcMainView->m_nLastSel );
	}

	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Select( int nRow, bool bReplace, bool bSelect )
{
	Select( nRow, nRow, bReplace, bSelect );
}

void BListView::ClearSelection()
{
	if ( m_pcMainView->ClearSelection() )
	{
		SelectionChanged( -1, -1 );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Highlight( int nFirst, int nLast, bool bReplace, bool bHighlight )
{
	if ( nLast < 0 )
	{
		return;
	}

	if ( nFirst < 0 )
	{
		nFirst = 0;
	}

	if ( nFirst >= int(m_pcMainView->m_cRows.size()) )
	{
		return;
	}

	if ( nLast >= int(m_pcMainView->m_cRows.size()) )
	{
		nLast = m_pcMainView->m_cRows.size() - 1;
	}

	if ( bReplace )
	{
		for ( int i = 0 ; i < int( m_pcMainView->m_cRows.size() ) ; ++i )
		{
			bool bHigh = (i >= nFirst && i <= nLast);
			if ( m_pcMainView->m_cRows[i]->m_bHighlighted != bHigh )
			{
				m_pcMainView->m_cRows[i]->m_bHighlighted = bHigh;
				m_pcMainView->InvalidateRow( i, BListView::INV_VISUAL );
			}
		}
	}
	else
	{
		for ( int i = nFirst ; i <= nLast ; ++i )
		{
			if ( m_pcMainView->m_cRows[i]->m_bHighlighted != bHighlight )
			{
				m_pcMainView->m_cRows[i]->m_bHighlighted = bHighlight;
				m_pcMainView->InvalidateRow( i, BListView::INV_VISUAL );
			}
		}
	}

	Flush();
}

void BListView::SetCurrentRow( int nRow )
{
	if ( m_pcMainView->m_cRows.empty() )
	{
		return;
	}

	if ( nRow < 0 )
	{
		nRow = 0;
	}
	else if ( nRow >= int(m_pcMainView->m_cRows.size()) )
	{
		nRow = m_pcMainView->m_cRows.size() - 1;
	}

	m_pcMainView->m_nEndSel = nRow;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Highlight( int nRow, bool bReplace, bool bHighlight )
{
	Highlight( nRow, nRow, bReplace, bHighlight );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool BListView::IsSelected( uint nRow ) const
{
	if ( nRow < m_pcMainView->m_cRows.size() )
	{
		return( m_pcMainView->m_cRows[nRow]->fSelected );
	}

	return( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Clear()
{
	m_pcMainView->Clear();
	AdjustScrollBars();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Draw( BRect cUpdateRect )
{
	if ( m_pcVScroll != NULL && m_pcHScroll != NULL )
	{
		BRect cHFrame = m_pcHScroll->Frame();
		BRect cVFrame = m_pcVScroll->Frame();
		BRect cFrame( cHFrame.right + 1, cVFrame.bottom + 1, cVFrame.right, cHFrame.bottom );
		FillRect( cFrame, ui_color(B_PANEL_BACKGROUND_COLOR) );
	}

	if ( m_pcMainView->m_nModeFlags & F_RENDER_BORDER )
	{
		DrawFrame( BRect(Bounds()), FRAME_RECESSED | FRAME_TRANSPARENT );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::Invoked( int nFirstRow, int nLastRow )
{
	if ( m_pcInvokeMsg != NULL )
	{
		Invoke( m_pcInvokeMsg );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::SelectionChanged( int nFirstRow, int nLastRow )
{
	if ( m_pcSelChangeMsg != NULL )
	{
		Invoke( m_pcSelChangeMsg );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int BListView::GetFirstSelected() const
{
	return( m_pcMainView->m_nFirstSel );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::StartScroll( scroll_direction eDirection, bool bSelect )
{
	m_pcMainView->StartScroll( eDirection, bSelect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void BListView::StopScroll()
{
	m_pcMainView->StopScroll();
}
  
//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int BListView::GetLastSelected() const
{
	return( m_pcMainView->m_nLastSel );
}

bool BListView::DragSelection( const BPoint& cPos )
{
	return( false );
}


BListView::const_iterator BListView::begin() const
{
	return( m_pcMainView->m_cRows.begin() );
}

BListView::const_iterator BListView::end() const
{
	return( m_pcMainView->m_cRows.end() );
}

void BListView::__reserved1__() {}
void BListView::__reserved2__() {}
void BListView::__reserved3__() {}
void BListView::__reserved4__() {}
