////////////////////////////////////////////////////////////////////////////////
//
//  File:           StringItem.h
//
//  Description:
//
//  Copyright 2001, Ulrich Wimboeck
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _STRING_ITEM_H
#define _STRING_ITEM_H

#include <ListItem.h>


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif


//----- BStringItem class ----------------------------------------

class BStringItem : public BListItem
{
public:
	BStringItem() {}
	virtual ~BStringItem() {}

	void				AttachToView( BView* pcView, int nColumn );
	void				SetRect( const BRect& cRect, int nColumn );
	void				AppendString( const std::string& cString );
	const std::string&	GetString( int nIndex ) const;
	virtual float 		Width( BView* pcView, int nColumn );
	virtual float  		Height( BView* pcView );
	virtual void 		Draw( const BRect& cFrame, BView* pcView, uint nColumn,
								bool bSelected, bool bHighlighted, bool bHasFocus );
	virtual bool		IsLessThan( const BListItem* pcOther, uint nColumn ) const;

private:
    std::vector< std::pair<std::string,float> > m_cStrings;
	uint32	_reserved[2];
};

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif /* _STRING_ITEM_H */
