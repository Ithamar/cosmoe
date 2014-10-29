/*******************************************************************************
/
/	File:			ListItem.h
/
/   Description:    BListView represents a one-dimensional list view. 
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _LIST_ITEM_H
#define _LIST_ITEM_H

#include <BeBuild.h>
#include <Archivable.h>
#include <Rect.h>

class BFont;
class BMessage;
class BOutlineListView;
class BView;

/*----------------------------------------------------------------*/
/*----- BListItem class ------------------------------------------*/

class BListItem
{
public:
    BListItem();
virtual				~BListItem();

    virtual void	AttachToView( BView* pcView, int nColumn ) = 0;
    virtual void	SetRect( const BRect& cRect, int nColumn ) = 0;

    virtual float	Width( BView* pcView, int nColumn ) = 0;
    virtual float	Height( BView* pcView ) = 0;
    virtual void	Draw( const BRect& cFrame, BView* pcView, uint nColumn,
						  bool bSelected, bool bHighlighted, bool bHasFocus ) = 0;
    virtual bool	HitTest( BView* pcView, const BRect& cFrame, int nColumn, BPoint cPos );
    virtual bool	IsLessThan( const BListItem* pcOther, uint nColumn ) const = 0;

    void    		SetIsSelectable( bool bSelectable );
    bool			IsSelectable() const;
    bool			IsSelected() const;
    bool			IsHighlighted() const;

/*----- Private or reserved -----------------------------------------*/
private:
    friend class BListView;
    friend class ListViewContainer;
    friend class ListViewCol;
    friend class std::vector<BListItem>;
    friend struct RowPosPred;
    
		uint32		_reserved[2];
float	m_vYPos;
		float		fHeight;
    bool	m_bIsSelectable;
		bool		fSelected;
    bool	m_bHighlighted;
};


/*-------------------------------------------------------------*/

#include <StringItem.h> /* to maintain compatibility */

#endif /* _LIST_ITEM_H */
