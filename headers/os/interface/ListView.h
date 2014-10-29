//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//	Distributed under the terms of the OpenBeOS license
//
//	File:			ListView.h
//	Author:			Axel Doerfler
//  Description:    BListView represents a one-dimensional list view.
//
//------------------------------------------------------------------------------

#ifndef _LIST_VIEW_H
#define _LIST_VIEW_H

#include <Control.h>
#include <ListItem.h>
#include <StringItem.h>

#include <vector>
#include <string>
#include <functional>



class BScrollBar;
class Button;
class BMessage;

class ListViewContainer;

typedef std::vector<int>			      column_map;





//----- BListView class ------------------------------------------

class BListView : public BControl
{
public:
	enum scroll_direction { SCROLL_UP, SCROLL_DOWN };
	enum { AUTOSCROLL_BORDER = 20 };
	enum {	F_MULTI_SELECT = 0x0001,
			F_NO_AUTO_SORT = 0x0002,
			F_RENDER_BORDER = 0x0004,
			F_DONT_SCROLL = 0x0008,
			F_NO_HEADER = 0x0010,
			F_NO_COL_REMAP = 0x0020 };

	enum {	INV_HEIGHT = 0x01,
			INV_WIDTH  = 0x02,
			INV_VISUAL = 0x04 };

	typedef std::vector<BListItem*>::const_iterator const_iterator;

	BListView( BRect inFrame, const char* inTitle, uint32 inModeFlags = F_MULTI_SELECT | F_RENDER_BORDER,
		uint32 inResizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 inFlags = B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE );

	~BListView();

	virtual void	LabelChanged( const std::string& cNewLabel );
	virtual void	EnableStatusChanged( bool bIsEnabled );

	virtual void	Invoked( int nFirstRow, int nLastRow );
	virtual void	SelectionChanged( int nFirstRow, int nLastRow );
	virtual bool	DragSelection( const BPoint& cPos );

	virtual void __reserved1__();
	virtual void __reserved2__();
	virtual void __reserved3__();
	virtual void __reserved4__();

	void			StartScroll( scroll_direction eDirection, bool bSelect );
	void			StopScroll();

	bool			IsMultiSelect() const;
	void			SetMultiSelect( bool bMulti );

	bool			IsAutoSort() const;
	void			SetAutoSort( bool bAuto );

	bool			HasBorder() const;
	void			SetRenderBorder( bool bRender );

	bool			HasColumnHeader() const;
	void			SetHasColumnHeader( bool bFlag );

	void			MakeVisible( int nRow, bool bCenter = true );
	int				InsertColumn( const char* pzTitle, int nWidth, int nPos = -1 );

	const column_map&	GetColumnMapping() const;
	void			SetColumnMapping( const column_map& cMap );

	void			InsertRow( int nPos, BListItem* pcRow, bool bUpdate = true );
	void			InsertRow( BListItem* pcRow, bool bUpdate = true );
	BListItem*	RemoveRow( int nIndex, bool bUpdate = true );
	void			InvalidateRow( int nRow, uint32 nFlags );
	uint			GetRowCount() const;
	BListItem*	GetRow( const BPoint& cPos ) const;
	BListItem*	GetRow( uint nIndex ) const;
	int				HitTest( const BPoint& cPos ) const;
	float			GetRowPos( int nRow );
	void			Clear();
	bool			IsSelected( uint nRow ) const;
	void			Select( int nFirst, int nLast, bool bReplace = true, bool bSelect = true );
	void			Select( int nRow, bool bReplace = true, bool bSelect = true );
	void			ClearSelection();

	void			Highlight( int nFirst, int nLast, bool bReplace, bool bHighlight = true );
	void			Highlight( int nRow, bool bReplace, bool bHighlight = true );

	void			SetCurrentRow( int nRow );
	void			Sort();
	int				GetFirstSelected() const;
	int				GetLastSelected() const;
	void	 		SetSelChangeMsg( BMessage* pcMsg );
	void	 		SetInvokeMsg( BMessage* pcMsg );
	BMessage*		GetSelChangeMsg() const;
	BMessage*		GetInvokeMsg() const;
	virtual void	Draw( BRect cUpdateRect );
	virtual void	FrameResized( float inWidth, float inHeight );
	virtual void	KeyDown(const char* bytes, int32 numBytes);
	virtual void	AllAttached();
	virtual bool	IsFocus( void ) const;

	// STL iterator interface to the rows.
	const_iterator	begin() const;
	const_iterator	end() const;

private:
	friend class ListViewContainer;
	friend class ListViewHeader;
	void	Layout();
	void  	AdjustScrollBars( bool bOkToHScroll = true );

	ListViewContainer* m_pcMainView;
	ListViewHeader*    m_pcHeaderView;
	BScrollBar*	       m_pcVScroll;
	BScrollBar*	       m_pcHScroll;
	BMessage*	       m_pcSelChangeMsg;
	BMessage*	       m_pcInvokeMsg;
//    uint32	       m_nModeFlags;
};



#endif // __LISTVIEW_H__
