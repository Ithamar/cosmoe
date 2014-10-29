//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Layer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
 
#include <string.h>
#include <SupportDefs.h>
#include "Layer.h"
#include "ServerWindow.h"
#include "RGBColor.h"
#include "ServerCursor.h"
#include "CursorManager.h"
#include "DisplayDriver.h"
#include <stdio.h>

#include "Desktop.h"

#include "AppServer.h"
#include "ServerFont.h"
#include <ServerBitmap.h>

#include <clipping.h>
#include <macros.h>

#include <View.h>
#include <InterfaceDefs.h>
#include <Font.h>
#include <Locker.h>
#include <Messenger.h>
#include <Message.h>

Gate g_cLayerGate("layer_gate");

//#define DEBUG_LAYER
#ifdef DEBUG_LAYER
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

//#define DEBUG_LAYER_REBUILD
#ifdef DEBUG_LAYER_REBUILD
#	define RBTRACE(x) printf x
#else
#	define RBTRACE(x) ;
#endif

void Layer::Init()
{
	_visible          = NULL;
	m_pcPrevVisibleReg      = NULL;
	m_pcDrawReg             = NULL;
	_invalid           = NULL;
	m_pcActiveDamageReg     = NULL;
	_full             = NULL;
	m_pcPrevFullReg         = NULL;
	fIsUpdating           = false;
	_regions_invalid       = true;
	fLevel                = 0;
	fLayerData		= new LayerData;
	m_cFontViewListIterator = NULL;
	m_bIsAddedToFont        = false;

	_hidden        = 0;

	fBoundsLeftTop.Set( 0.0f, 0.0f );

	fUpperSibling	= NULL;
	fLowerSibling	= NULL;
	fTopChild		= NULL;
	fBottomChild	= NULL;

	m_pcBitmap = (NULL == fParent) ? NULL : fParent->m_pcBitmap;

	m_pcFont             = NULL;
	fLayerData->draw_mode       = B_OP_COPY;
	m_nRegionUpdateCount = 0;
	m_nMouseOffCnt       = 0;
	m_bIsMouseOn         = true;
	m_bFontPalletteValid = false;
	m_hHandle = g_pcLayers->Insert( this );
	m_bBackdrop = false;

	memset( m_asFontPallette, 255, sizeof( m_asFontPallette ) );
}

/*!
	\brief Constructor
	\param frame Size and placement of the Layer
	\param name Name of the layer
	\param resize Resizing flags as defined in View.h
	\param flags BView flags as defined in View.h
	\param win ServerWindow to which the Layer belongs
*/
Layer::Layer(BRect frame, const char* name, Layer* pcParent,
              int32 flags, void* pUserObj, ServerWindow* win ) : m_cName( name ), m_cIFrame(frame), fFrame(frame) 
{
	fServerWin          = win;
	fParent          = pcParent;
	m_pUserObject = pUserObj;
	_flags          = flags;

	isRootLayer = false;
	Init();

	if ( NULL != pcParent )
	{
		pcParent->AddChild( this, true );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC: Class constructor. Used for initializing of the screens top widget.
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Layer::Layer(ServerBitmap* pcBitmap, bool root) : m_cName( "bitmap_layer" )
{
	fServerWin = NULL;
	m_cIFrame  = IRect( 0, 0, pcBitmap->Width() - 1, pcBitmap->Height() - 1 );
	fFrame   = m_cIFrame.AsBRect();
	fParent = NULL;
	_flags   = 0;
	isRootLayer = root;
	Init();
	SetBitmap( pcBitmap );
}

//! Destructor frees all allocated heap space
Layer::~Layer(void)
{
	Layer*        pcChild;
	Layer*        pcNext;

	for ( pcChild = fTopChild ; NULL != pcChild ; pcChild = pcNext )
	{
		pcNext = pcChild->fLowerSibling;
		delete pcChild;
	}
	g_pcLayers->Remove( m_hHandle );
	DeleteRegions();

	if ( m_pcFont != NULL )
	{
		if ( m_bIsAddedToFont )
		{
			m_pcFont->RemoveDependency( m_cFontViewListIterator );
		}
		m_pcFont->Release();
	}
	if(fLayerData)
	{
		delete fLayerData;
		fLayerData=NULL;
	}
}

/*!
	\brief Adds a child to the back of the a layer's child stack.
	\param layer The layer to add as a child
	\param before Add the child in front of this layer
	\param rebuild Flag to fully rebuild all visibility regions
*/
void Layer::AddChild(Layer *layer, bool bTopmost)
{
	if ( NULL == fBottomChild && NULL == fTopChild )
	{
		fBottomChild            = layer;
		fTopChild               = layer;
		layer->fLowerSibling  = NULL;
		layer->fUpperSibling = NULL;
	}
	else
	{
		if ( bTopmost )
		{
			if ( layer->m_bBackdrop == false )
			{
				if ( NULL != fTopChild )
				{
					fTopChild->fUpperSibling = layer;
				}
				__assertw( fTopChild != NULL );
				layer->fLowerSibling = fTopChild;
				fTopChild              = layer;
				layer->fUpperSibling= NULL;
			}
			else
			{
				Layer* pcTmp;
				for ( pcTmp = fBottomChild ; pcTmp != NULL ; pcTmp = pcTmp->fUpperSibling )
				{
					if ( pcTmp->m_bBackdrop == false )
					{
						break;
					}
				}
				if ( pcTmp != NULL )
				{
					layer->fUpperSibling = pcTmp;
					layer->fLowerSibling  = pcTmp->fLowerSibling;
					pcTmp->fLowerSibling = layer;
					if ( layer->fLowerSibling != NULL )
					{
						layer->fLowerSibling->fUpperSibling = layer;
					}
					else
					{
						fBottomChild = layer;
					}
				}
				else
				{
					fTopChild->fUpperSibling = layer;
					layer->fLowerSibling           = fTopChild;
					fTopChild                              = layer;
					layer->fUpperSibling          = NULL;
				}
			}
		}
		else
		{
			if ( layer->m_bBackdrop )
			{
				if ( NULL != fBottomChild )
				{
					fBottomChild->fLowerSibling = layer;
				}
				__assertw( fBottomChild != NULL );
				layer->fUpperSibling = fBottomChild;
				fBottomChild                 = layer;
				layer->fLowerSibling         = NULL;
			}
			else
			{
				Layer* pcTmp;
				for ( pcTmp = fBottomChild ; pcTmp != NULL ; pcTmp = pcTmp->fUpperSibling )
				{
					if ( pcTmp->m_bBackdrop == false )
					{
						break;
					}
				}
				if ( pcTmp != NULL )
				{
					layer->fUpperSibling = pcTmp;
					layer->fLowerSibling  = pcTmp->fLowerSibling;
					pcTmp->fLowerSibling = layer;
					if ( layer->fLowerSibling != NULL )
					{
						layer->fLowerSibling->fUpperSibling = layer;
					}
					else
					{
						fBottomChild = layer;
					}
				}
				else
				{
					fTopChild->fUpperSibling = layer;
					layer->fLowerSibling           = fTopChild;
					fTopChild                              = layer;
					layer->fUpperSibling          = NULL;
				}
			}
		}
	}
	layer->SetBitmap( m_pcBitmap );
	layer->fParent = this;

	layer->Added( _hidden );        // Alloc clipping regions
}

/*!
	\brief Removes a layer from the child stack
	\param layer The layer to remove
	\param rebuild Flag to rebuild all visibility regions
*/
void Layer::RemoveChild(Layer *layer, bool rebuild)
{
	STRACE(("Layer(%s)::RemoveChild(%s) START\n", GetName(), layer->GetName()));
	
	if( layer->fParent == NULL )
	{
		printf("ERROR: RemoveChild(): Layer doesn't have a fParent\n");
		return;
	}
	
	if( layer->fParent != this )
	{
		printf("ERROR: RemoveChild(): Layer is not a child of this layer\n");
		return;
	}


	// Take care of fParent
	layer->fParent		= NULL;
	if( fTopChild == layer )
		fTopChild		= layer->fLowerSibling;
	if( fBottomChild == layer )
		fBottomChild	= layer->fUpperSibling;

	// Take care of siblings
	if( layer->fUpperSibling != NULL )
		layer->fUpperSibling->fLowerSibling	= layer->fLowerSibling;
	if( layer->fLowerSibling != NULL )
		layer->fLowerSibling->fUpperSibling = layer->fUpperSibling;
	layer->fUpperSibling	= NULL;
	layer->fLowerSibling	= NULL;

	layer->SetBitmap( NULL );

	if(rebuild)
		RebuildRegions(true);
}

/*!
	\brief Removes the layer from its parent's child stack
	\param rebuild Flag to rebuild visibility regions
*/
void Layer::RemoveSelf(bool rebuild)
{
	// A Layer removes itself from the tree (duh)
	if( fParent == NULL )
	{
		printf("ERROR: RemoveSelf(): Layer doesn't have a fParent\n");
		return;
	}
	Layer *p=fParent;
	fParent->RemoveChild(this);
	
	if(rebuild)
		p->RebuildRegions(true);
}

/*!
	\brief Finds the first child at a given point.
	\param pt Point to look for a child
	\param recursive Flag to look for the bottom-most child
	\return non-NULL if found, NULL if not

	Find out which child gets hit if we click at a certain spot. Returns NULL
	if there are no _visible children or if the click does not hit a child layer
	If recursive==true, then it will continue to call until it reaches a layer
	which has no children, i.e. a layer that is at the top of its 'branch' in
	the layer tree
*/
Layer *Layer::GetChildAt(BPoint pt, bool recursive)
{
	Layer *child;
	if(recursive)
	{
		for(child=fTopChild; child!=NULL; child=child->fLowerSibling)
		{
			if(child->fTopChild!=NULL)
				child->GetChildAt(pt,true);
			
			if(child->_hidden)
				continue;
			
			if(child->fFrame.Contains(pt))
				return child;
		}
	}
	else
	{
		for(child=fTopChild; child!=NULL; child=child->fLowerSibling)
		{
			if(child->_hidden)
				continue;
			if(child->fFrame.Contains(pt))
				return child;
		}
	}
	return NULL;
}

/*!
	\brief Returns the size of the layer
	\return the size of the layer
*/
BRect Layer::Bounds(void) const
{
	BRect r(fFrame);
	r.OffsetTo( fBoundsLeftTop );
	return r;
}

/*!
	\brief Returns the layer's size and position in its parent coordinates
	\return The layer's size and position in its parent coordinates
*/
BRect Layer::Frame(void) const
{
	return fFrame;
}


/*!
	\brief recursively deletes all children (and grandchildren, etc) of the layer

	This is mostly used for server shutdown or deleting a workspace
*/
void Layer::PruneTree(void)
{
	Layer *lay;
	Layer *nextlay;
	
	lay = fTopChild;
	fTopChild = NULL;
	
	while(lay != NULL)
	{
		if(lay->fTopChild != NULL)
			lay->PruneTree();

		nextlay = lay->fLowerSibling;
		lay->fLowerSibling = NULL;

		delete lay;
		lay = nextlay;
	}
	// Man, this thing is short. Elegant, ain't it? :P
}


/*!
	\brief Finds a layer based on its token ID
	\return non-NULL if found, NULL if not
*/
Layer *FindLayer(int32 token)
{
	// recursive search for a layer based on its view token
	Layer *lay = g_pcLayers->GetObj( token );
	return lay;
}

/*!
	\brief Sets a region as invalid and, thus, needing to be drawn
	\param The region to invalidate
	
	All children of the layer also receive this call, so only 1 Invalidate call is 
	needed to set a section as invalid on the screen.
*/


/*!
	\brief Sets a rectangle as invalid and, thus, needing to be drawn
	\param The rectangle to invalidate
	
	All children of the layer also receive this call, so only 1 Invalidate call is 
	needed to set a section as invalid on the screen.
*/
void Layer::Invalidate(const BRect &rect)
{
	// Make our own section dirty and pass it on to any children, if necessary....
	// YES, WE ARE SHARING DIRT! Mudpies anyone? :D
	if ( _hidden > 0 )
		return;
	{
		if(_invalid)
			_invalid->Include(rect);
		else
			_invalid=new BRegion(rect);
	}
}


void Layer::Invalidate( bool bRecursive )
{
	if ( _hidden == 0 )
	{
		delete _invalid;
		_invalid = new BRegion(Bounds());
		if ( bRecursive )
		{
			for ( Layer* pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
			{
				pcChild->Invalidate( true );
			}
		}
	}
}


/*!
	\brief Determines whether the layer needs to be redrawn
	\return True if the layer needs to be redrawn, false if not
*/
bool Layer::IsDirty(void) const
{
	//return (!_invalid)?true:false;
	return _is_dirty;
}

/*!
	\brief Forces a repaint if there are invalid areas
	\param force_update Force an update. False by default.
*/
void Layer::UpdateIfNeeded(bool force_update)
{
	if(IsHidden())
		return;

	Layer *child;

	if ( fServerWin != NULL && ((fServerWin->GetWorkspaceIndex() & (1 << get_active_desktop())) == 0 ||
								fServerWin->HasPendingSizeEvents( this )) )
	{
		return;
	}

	if (_invalid)
	{
		if ( m_pcActiveDamageReg == NULL )
		{
			m_pcActiveDamageReg = _invalid;
			_invalid = NULL;
			RequestDraw( (m_pcActiveDamageReg->Frame()), true );
		}
	}

	for(child=fBottomChild; child!=NULL; child=child->fUpperSibling)
		child->UpdateIfNeeded(force_update);
	_is_dirty=false;
}


/*!
	\brief Marks the layer as needing a region rebuild if intersecting the given rect
	\param rect The rectangle for checking to see if the layer needs to be rebuilt
*/
void Layer::MarkModified(BRect rect)
{
	if (Bounds().Contains(rect))
	{
		_regions_invalid = true;

		Layer *child;
		for (child = fBottomChild ; child!=NULL; child=child->fUpperSibling )
			child->MarkModified(rect.OffsetByCopy(-child->fFrame.left,-child->fFrame.top));
	}
}

/*!
	\brief Rebuilds the layer's regions and updates the screen as needed
	\param force Force an update
*/
void Layer::UpdateRegions(bool force, bool bRoot)
{
	RebuildRegions(force);
	MoveChildren();
	InvalidateNewAreas();

	if (_regions_invalid && fParent == NULL && m_pcBitmap != NULL && _invalid != NULL )
	{
		if (isRootLayer)
		{
			RGBColor  sColor;
			BPoint    cTopLeft( ConvertToRoot( BPoint( 0, 0 ) ) );

			sColor.SetColor(0x00, 0x60, 0x6b, 0);

			BRegion cTmpReg(Bounds());
			BRegion cDrawReg( *_visible );
			BRect   aRect;

			cTmpReg.Exclude( _invalid );
			cDrawReg.Exclude( &cTmpReg );

			cDrawReg.OffsetBy(cTopLeft.x, cTopLeft.y);
			m_pcBitmap->m_pcDriver->FillRegion(cDrawReg, sColor);
		}
		delete _invalid;
		_invalid = NULL;
	}
	UpdateIfNeeded(false);
	ClearDirtyRegFlags();
}


//! Show the layer. Operates just like the BView call with the same name
void Layer::Show(void)
{
	ShowHideHelper(true);
}

//! Hide the layer. Operates just like the BView call with the same name
void Layer::Hide(void)
{
	ShowHideHelper(false);
}

/*!
	\brief Determines whether the layer is hidden or not
	\return true if hidden, false if not.
*/
bool Layer::IsHidden(void) const
{
	return _hidden;
}

/*!
	\brief Counts the number of children the layer has
	\return the number of children the layer has, not including grandchildren
*/
uint32 Layer::CountChildren(void) const
{
	uint32 i=0;
	Layer *lay=fTopChild;
	while(lay!=NULL)
	{
		lay=lay->fLowerSibling;
		i++;
	}
	return i;
}

/*!
	\brief Moves a layer in its parent coordinate space
	\param x X offset
	\param y Y offset
*/

/*!
	\brief Resizes the layer.
	\param x X offset
	\param y Y offset
	
	This resizes the layer itself and resizes any children based on their resize
	flags.
*/

/*!
	\brief Rebuilds visibility regions for child layers
	\param include_children Flag to rebuild all children and subchildren
*/
void Layer::RebuildRegions(bool bForce)
{
	Layer*        pcSibling;
	Layer*        pcChild;

	if ( fServerWin != NULL && (fServerWin->GetWorkspaceIndex() & (1 << get_active_desktop())) == 0 ) {
		return;
	}

	if ( _hidden > 0 )
	{
		if ( _visible != NULL )
		{
			DeleteRegions();
		}
		return;
	}

	if ( bForce )
	{
		_regions_invalid = true;
	}

	if ( _regions_invalid )
	{
		delete m_pcDrawReg;
		m_pcDrawReg = NULL;

		__assertw( m_pcPrevVisibleReg == NULL );
		__assertw( m_pcPrevFullReg == NULL );

		m_pcPrevVisibleReg = _visible;
		m_pcPrevFullReg    = _full;

		if ( NULL == fParent )
		{
			_full = new BRegion(fFrame);
		}
		else
		{
			__assertw( fParent->_full != NULL );
			if ( fParent->_full != NULL )
			{
				_full = new BRegion(*fParent->_full);
				BRegion clipReg(fFrame);
				_full->IntersectWith(&clipReg);
				_full->OffsetBy(-fFrame.left, -fFrame.top);
			}
			else
			{
				_full = new BRegion(fFrame);
			}
		}
//    if ( fLevel == 1 ) {
		BPoint cLeftTop( fFrame.LeftTop() );
		for ( pcSibling = fUpperSibling ; NULL != pcSibling ; pcSibling = pcSibling->fUpperSibling ) {
			if ( pcSibling->_hidden == 0 )
			{
				if ( pcSibling->fFrame.Contains( fFrame ) )
				{
					_full->Exclude( (pcSibling->fFrame.OffsetByCopy(-cLeftTop)) );
				}
			}
		}
//    }

		_visible = new BRegion( *_full );

		if ( (_flags & B_DRAW_ON_CHILDREN) == 0 )
		{
			bool bRegModified = false;
			for ( pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
			{
				// Remove children from child region
				if ( pcChild->_hidden == 0 )
				{
					_visible->Exclude( pcChild->fFrame );
					bRegModified = true;
				}
			}
		}
	}

	// Rebuild the regions for subchildren if we're supposed to
	for ( pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling ) {
		pcChild->RebuildRegions( bForce );
	}
}


//! Prints all relevant layer data to stdout
void Layer::PrintToStream(void)
{
	printf("-----------\nLayer %s\n",fName->String());
	if(fParent)
		printf("Parent: %s (%p)\n",fParent->fName->String(), fParent);
	else
		printf("Parent: NULL\n");
	if(fUpperSibling)
		printf("Upper sibling: %s (%p)\n",fUpperSibling->fName->String(), fUpperSibling);
	else
		printf("Upper sibling: NULL\n");
	if(fLowerSibling)
		printf("Lower sibling: %s (%p)\n",fLowerSibling->fName->String(), fLowerSibling);
	else
		printf("Lower sibling: NULL\n");
	if(fTopChild)
		printf("Top child: %s (%p)\n",fTopChild->fName->String(), fTopChild);
	else
		printf("Top child: NULL\n");
	if(fBottomChild)
		printf("Bottom child: %s (%p)\n",fBottomChild->fName->String(), fBottomChild);
	else
		printf("Bottom child: NULL\n");
	printf("Frame: "); fFrame.PrintToStream();
	printf("Token: %ld\nLevel: %ld\n",fViewToken, fLevel);
	printf("Hide count: %s\n",_hidden?"true":"false");
	if(_invalid)
	{
		printf("Invalid Areas: "); _invalid->PrintToStream();
	}
	else
		printf("Invalid Areas: NULL\n");
	if(_visible)
	{
		printf("Visible Areas: "); _visible->PrintToStream();
	}
	else
		printf("Visible Areas: NULL\n");
	printf("Is updating = %s\n",(fIsUpdating)?"yes":"no");
}

//! Prints hierarchy data to stdout
void Layer::PrintNode(void)
{
	printf("-----------\nLayer %s\n",fName->String());
	if(fParent)
		printf("Parent: %s (%p)\n",fParent->fName->String(), fParent);
	else
		printf("Parent: NULL\n");
	if(fUpperSibling)
		printf("Upper sibling: %s (%p)\n",fUpperSibling->fName->String(), fUpperSibling);
	else
		printf("Upper sibling: NULL\n");
	if(fLowerSibling)
		printf("Lower sibling: %s (%p)\n",fLowerSibling->fName->String(), fLowerSibling);
	else
		printf("Lower sibling: NULL\n");
	if(fTopChild)
		printf("Top child: %s (%p)\n",fTopChild->fName->String(), fTopChild);
	else
		printf("Top child: NULL\n");
	if(fBottomChild)
		printf("Bottom child: %s (%p)\n",fBottomChild->fName->String(), fBottomChild);
	else
		printf("Bottom child: NULL\n");
	if(_visible)
	{
		printf("Visible Areas: "); _visible->PrintToStream();
	}
	else
		printf("Visible Areas: NULL\n");
}

//! Prints the tree structure starting from this layer
void Layer::PrintTree(){

	int32		spaces = 2;
	Layer		*c = fTopChild; //c = short for: current
	printf( "'%s' - token: %ld\n", fName->String(), fViewToken );
	if( c != NULL )
		while( true ){
			// action block
			{
				for( int i = 0; i < spaces; i++)
					printf(" ");
				
					printf( "'%s' - token: %ld\n", c->fName->String(), c->fViewToken );
			}

				// go deep
			if(	c->fTopChild ){
				c = c->fTopChild;
				spaces += 2;
			}
				// go right or up
			else
					// go right
				if( c->fLowerSibling ){
					c = c->fLowerSibling;
				}
					// go up
				else{
					while( !c->fParent->fLowerSibling && c->fParent != this ){
						c = c->fParent;
						spaces -= 2;
					}
						// that enough! We've reached this view.
					if( c->fParent == this )
						break;
						
					c = c->fParent->fLowerSibling;
					spaces -= 2;
				}
		}
}

void Layer::ShowHideHelper( bool bFlag )
{
	if ( fParent == NULL || fServerWin == NULL )
	{
		printf( "Error: Layer::Show() attempt to hide root layer\n" );
		return;
	}

	if ( bFlag )
	{
		_hidden--;
	}
	else
	{
		_hidden++;
	}

	fParent->_regions_invalid = true;
	SetDirtyRegFlags();

	Layer* pcSibling;
	for ( pcSibling = fParent->fBottomChild ; pcSibling != NULL ; pcSibling = pcSibling->fUpperSibling )
	{
		if (pcSibling->fFrame.Contains(fFrame))
		{
			pcSibling->MarkModified(fFrame.OffsetByCopy(-pcSibling->fFrame.LeftTop()));
		}
	}

	Layer* pcChild;
	for ( pcChild = fTopChild ; NULL != pcChild ; pcChild = pcChild->fLowerSibling )
	{
		pcChild->ShowHideHelper( bFlag );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetWindow( ServerWindow* pcWindow )
{
	fServerWin = pcWindow;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------


void Layer::Added( int nHideCount )
{
	Layer* pcChild;

	_hidden += nHideCount;
	if ( fParent != NULL )
	{
		fLevel = fParent->fLevel + 1;
	}

	for ( pcChild = fTopChild ; NULL != pcChild ; pcChild = pcChild->fLowerSibling )
	{
		pcChild->Added( nHideCount );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::DeleteRegions()
{
	__assertw( m_pcPrevVisibleReg == NULL );
	__assertw( m_pcPrevFullReg == NULL );

	delete _visible;
	delete _full;
	delete _invalid;
	delete m_pcActiveDamageReg;
	delete m_pcDrawReg;

	_visible      = NULL;
	_full         = NULL;
	m_pcDrawReg         = NULL;
	_invalid       = NULL;
	m_pcActiveDamageReg = NULL;
	m_pcDrawReg         = NULL;

	Layer* pcChild;
	for ( pcChild = fTopChild ; NULL != pcChild ; pcChild = pcChild->fLowerSibling )
	{
		pcChild->DeleteRegions();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetBitmap( ServerBitmap* pcBitmap )
{
	Layer* pcChild;

	m_bFontPalletteValid  = false;
	m_pcBitmap = pcBitmap;

	for ( pcChild = fTopChild ; NULL != pcChild ; pcChild = pcChild->fLowerSibling )
	{
		pcChild->SetBitmap( pcBitmap );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Layer::ToggleDepth()
{
	if ( fParent != NULL )
	{
		Layer* pcParent = fParent;

		if ( pcParent->fTopChild == this )
		{
			pcParent->RemoveChild( this );
			pcParent->AddChild( this, false );
		}
		else
		{
			pcParent->RemoveChild( this );
			pcParent->AddChild( this, true );
		}

		fParent->_regions_invalid = true;
		SetDirtyRegFlags();

		Layer* pcSibling;
		for ( pcSibling = fParent->fBottomChild ; pcSibling != NULL ; pcSibling = pcSibling->fUpperSibling ) {
			if ( pcSibling->fFrame.Contains( fFrame ) )
			{
				pcSibling->MarkModified(fFrame.OffsetByCopy(-pcSibling->fFrame.LeftTop()));
			}
		}
	}
	return( false );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
//        Mark the clipping region for ourself and all our children as modified.
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetDirtyRegFlags()
{
	_regions_invalid = true;

	Layer* pcChild;
	for ( pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
	{
		pcChild->SetDirtyRegFlags();
	}
}



void Layer::ScrollBy( const BPoint& cOffset )
{
	if ( fParent == NULL )
	{
		return;
	}

	IPoint cOldOffset = m_cIScrollOffset;
	m_cScrollOffset += cOffset;
	m_cIScrollOffset = IPoint(m_cScrollOffset);

	if ( m_cIScrollOffset == cOldOffset )
	{
		return;
	}

	IPoint cIOffset = m_cIScrollOffset - cOldOffset;

	Layer* pcChild;
	for ( pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
	{
		pcChild->m_cIFrame += cIOffset;
		pcChild->fFrame.OffsetBy(cIOffset.AsBPoint());
	}

	if ( _hidden > 0 )
	{
		return;
	}

	UpdateRegions();
	ServerWindow::HandleMouseTransaction();

	if ( NULL == _full || m_pcBitmap == NULL )
	{
		return;
	}

	BRect   cBounds = Bounds();
	BRect   srcRect;
	BRect   dstRect;
	BRegion cBltList;
	BRegion cDamage( *_visible );

	ENUMBRECTS((*_full), srcRect)
	{
		// Clip to source rectangle
		BRect cSRect = (cBounds & srcRect);

		// Transform into destination space

		if ( cSRect.IsValid() == false )
			continue;

		cSRect.OffsetBy(cOffset);

		ENUMBRECTS((*_full), dstRect )
		{
			BRect cDRect = (cSRect & dstRect);

			if ( cDRect.IsValid() == false )
				continue;

			cDamage.Exclude( cDRect );

			clipping_rect pcClip = to_clipping_rect(cDRect);
			pcClip.move_x = cIOffset.x;
			pcClip.move_y = cIOffset.y;

			cBltList.Include(pcClip);
		}
	}

	int nCount = cBltList.CountRects();

	if ( nCount == 0 )
	{
		Invalidate( cBounds );
		UpdateIfNeeded( true );
		return;
	}
	IPoint cTopLeft( ConvertToRoot( BPoint( 0, 0 ) ) );

	clipping_rect cliprect;

#if 0
	ENUMCLIPRECTS(cBltList, cliprect)
	{
		offset_rect(cliprect, cTopLeft.x, cTopLeft.y); // Convert into screen space
		IRect icliprect(to_BRect(cliprect));
		icliprect -= IPoint(cliprect.move_x, cliprect.move_y);
		m_pcBitmap->m_pcDriver->BltBitmap(m_pcBitmap,
										  m_pcBitmap,
										  icliprect,
										  IPoint(cliprect.left, cliprect.top),
										  B_OP_COPY);
	}
#else
	// TODO: Use DDriver->Blit()
#endif

	if ( _invalid != NULL )
	{
		_invalid->OffsetBy(cIOffset.x, cIOffset.y);
	}

	if ( m_pcActiveDamageReg != NULL )
	{
		m_pcActiveDamageReg->OffsetBy(cIOffset.x, cIOffset.y);
	}

	ENUMBRECTS(cDamage, dstRect)
	{
		Invalidate(dstRect);
	}

	UpdateIfNeeded(true);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetFrame( const BRect& cRect )
{
	IRect cIRect( cRect );
	fFrame = cRect;

	if ( _hidden == 0 )
	{
		if ( m_cIFrame == cIRect )
		{
			return;
		}
		m_cDeltaMove += IPoint( cIRect.left, cIRect.top ) - IPoint( m_cIFrame.left, m_cIFrame.top );
		m_cDeltaSize += IPoint( cIRect.Width(), cIRect.Height() ) - IPoint( m_cIFrame.Width(), m_cIFrame.Height() );

		if ( fParent != NULL )
		{
			fParent->_regions_invalid = true;
		}

		SetDirtyRegFlags();
		Layer* pcSibling;
		for ( pcSibling = fLowerSibling ; pcSibling != NULL ; pcSibling = pcSibling->fLowerSibling )
		{
			if ( pcSibling->fFrame.Contains(fFrame) || pcSibling->fFrame.Contains(cRect) )
			{
				pcSibling->MarkModified(fFrame.OffsetByCopy(-pcSibling->fFrame.LeftTop()));
				pcSibling->MarkModified(const_cast<BRect&>(cRect).OffsetByCopy(-pcSibling->fFrame.LeftTop()));
			}
		}
	}
	m_cIFrame = cIRect;
	if ( fParent == NULL )
	{
		Invalidate();
	}
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::InvalidateNewAreas( void )
{
	BRegion* pcRegion;

	if ( _hidden > 0 )
		return;

	if ( fServerWin != NULL && (fServerWin->GetWorkspaceIndex() & (1 << get_active_desktop())) == 0 )
		return;

	if ( _regions_invalid )
	{
		if	(
				(_flags & B_FULL_UPDATE_ON_RESIZE)
				&&
				((m_cDeltaSize.x != 0) || (m_cDeltaSize.y != 0))
			)
		{
			Invalidate( false );
		}
		else
		{
			if ( _visible != NULL )
			{
				pcRegion = new BRegion( *_visible );

				if ( pcRegion != NULL )
				{
					if ( m_pcPrevVisibleReg != NULL )
					{
						pcRegion->Exclude( m_pcPrevVisibleReg );
					}
					if ( _invalid == NULL ) {
						if ( pcRegion->CountRects() > 0 )
						{
							_invalid = pcRegion;
						}
						else
						{
							delete pcRegion;
						}
					}
					else
					{
						BRect clipRect;
						ENUMBRECTS((*pcRegion), clipRect )
						{
							Invalidate(clipRect);
						}
						delete pcRegion;
					}
				}
			}
		}
		delete m_pcPrevVisibleReg;
		m_pcPrevVisibleReg = NULL;

		m_cDeltaSize = IPoint( 0, 0 );
		m_cDeltaMove = IPoint( 0, 0 );
	}

	for ( Layer* pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
	{
//    if ( fLevel != 0 || pcChild->fServerWin == NULL ) {
		pcChild->InvalidateNewAreas();
//    }
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::MoveChildren()
{
	if ( _hidden > 0 || NULL == m_pcBitmap )
	{
		return;
	}

	if ( fServerWin != NULL && (fServerWin->GetWorkspaceIndex() & (1 << get_active_desktop())) == 0 )
	{
		return;
	}

	Layer* pcChild;
	if ( _regions_invalid )
	{
		BRect cBounds = Bounds();

		for ( pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
		{
			if ( pcChild->m_cDeltaMove.x == 0.0f && pcChild->m_cDeltaMove.y == 0.0f )
			{
				continue;
			}

			if ( pcChild->_full == NULL || pcChild->m_pcPrevFullReg == NULL )
			{
				continue;
			}

			BRegion* pcRegion = new BRegion( *pcChild->m_pcPrevFullReg );
			pcRegion->IntersectWith( pcChild->_full );

			int nCount = 0;

			IPoint cTopLeft( ConvertToRoot( BPoint( 0, 0 ) ) );
			BPoint cChildOffset(pcChild->fFrame.left + cTopLeft.x,
								pcChild->fFrame.top + cTopLeft.y);

			// Transform into parents coordinate system
			pcRegion->OffsetBy(cChildOffset.x, cChildOffset.y);
			pcRegion->SetMove(pcChild->m_cDeltaMove.x, pcChild->m_cDeltaMove.y);
			nCount += pcRegion->CountRects();

			if ( nCount == 0 )
			{
				delete pcRegion;
				continue;
			}

			clipping_rect cliprect;

			ENUMCLIPRECTS((*pcRegion), cliprect)
			{
#if 0
				IRect icliprect(to_BRect(cliprect));
				icliprect -= IPoint(cliprect.move_x, cliprect.move_y);
				
				m_pcBitmap->m_pcDriver->BltBitmap(m_pcBitmap,
												  m_pcBitmap,
												  icliprect,
												  IPoint(cliprect.left, cliprect.top),
												  B_OP_COPY );
#else
				BRect icliprect(to_BRect(cliprect));
				icliprect.OffsetBy(-cliprect.move_x, -cliprect.move_y);
				
				m_pcBitmap->m_pcDriver->Blit(icliprect, to_BRect(cliprect), NULL);
#endif
			}
			delete pcRegion;
		}
		/*        Since the parent window is shrinked before the children is moved
		*        we may need to redraw the right and bottom edges.
		*/
		if ( NULL != fParent && (m_cDeltaMove.x != 0.0f || m_cDeltaMove.y != 0.0f) ) {
			if ( fParent->m_cDeltaSize.x < 0 )
			{
				BRect        cRect(cBounds);

				cRect.left = cRect.right + int( fParent->m_cDeltaSize.x + fParent->fFrame.right -
												fParent->fFrame.left - fFrame.right );

				if ( cRect.IsValid() )
				{
					Invalidate( cRect );
				}
			}
			if ( fParent->m_cDeltaSize.y < 0 )
			{
				BRect        cRect(cBounds);

				cRect.top = cRect.bottom + int( fParent->m_cDeltaSize.y + fParent->fFrame.bottom -
												fParent->fFrame.top - fFrame.bottom );

				if ( cRect.IsValid() )
				{
					Invalidate( cRect );
				}
			}
		}
		delete m_pcPrevFullReg;
		m_pcPrevFullReg = NULL;
	}
	for ( pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling ) {
//    if ( fLevel != 0 || pcChild->fServerWin == NULL ) {
		pcChild->MoveChildren();
//    }
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
//        Stores the previous visible region in m_pcPrevVisibleReg and then
//        rebuilds _visible, starting with whatever is left of our parent
//        and then removing areas covered by siblings.
// NOTE:
//        Areas covered by children are not removed.
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SwapRegions( bool bForce )
{
	if ( bForce )
	{
		_regions_invalid = true;
	}

	Layer* pcChild;
	for ( pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
	{
		pcChild->SwapRegions( bForce );
	}
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::ClearDirtyRegFlags()
{
	_regions_invalid = false;
	for ( Layer* pcChild = fBottomChild ; NULL != pcChild ; pcChild = pcChild->fUpperSibling )
	{
//    if ( fLevel != 0 || pcChild->fServerWin == NULL ) {
		pcChild->ClearDirtyRegFlags();
//    }
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

BRegion* Layer::GetRegion()
{
	if ( _hidden > 0 )
	{
		return( NULL );
	}

	if ( fIsUpdating && m_pcActiveDamageReg == NULL )
	{
		return( NULL );
	}

	if ( fIsUpdating == false )
	{
		return( _visible );
	}

	if ( m_pcDrawReg == NULL && _visible != NULL )
	{
		BRegion cTmpReg(Bounds());

		m_pcDrawReg = new BRegion( *_visible );

		__assertw( m_pcActiveDamageReg != NULL );

		cTmpReg.Exclude( m_pcActiveDamageReg );

		m_pcDrawReg->Exclude( &cTmpReg );
	}
	__assertw( NULL != m_pcDrawReg );
	return( m_pcDrawReg );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::BeginUpdate()
{
	if ( _visible != NULL )
	{
		fIsUpdating = true;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::EndUpdate( void )
{
	delete m_pcActiveDamageReg;
	m_pcActiveDamageReg = NULL;
	delete m_pcDrawReg;
	m_pcDrawReg = NULL;
	fIsUpdating        = false;

	if ( _invalid != NULL )
	{
		m_pcActiveDamageReg = _invalid;
		_invalid = NULL;
		RequestDraw( (m_pcActiveDamageReg->Frame()), true );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetFont( FontNode* pcFont )
{
	if ( pcFont == m_pcFont )
	{
		return;
	}

	if ( pcFont != NULL )
	{
		pcFont->AddRef();
		if ( m_pcFont != NULL )
		{
			if ( m_bIsAddedToFont )
			{
				m_pcFont->RemoveDependency( m_cFontViewListIterator );
				m_bIsAddedToFont = false;
			}
			m_pcFont->Release();
		}
		m_pcFont = pcFont;
		if ( m_pUserObject != NULL && fServerWin != NULL && fServerWin->GetAppTarget() != NULL )
		{
			m_cFontViewListIterator = m_pcFont->AddDependency( fServerWin->GetAppTarget(), m_pUserObject );
			m_bIsAddedToFont = true;
		}
	}
	else
	{
		printf( "ERROR : Layer::SetFont() called with NULL pointer\n" );
	}
}


font_height Layer::GetFontHeight() const
{
	if ( m_pcFont != NULL )
	{
		ServerFont* pcFontInst = m_pcFont->GetInstance();
		if ( pcFontInst != NULL )
		{
			font_height sHeight;
			sHeight.ascent  = pcFontInst->GetAscender();
			sHeight.descent = -pcFontInst->GetDescender();
			sHeight.leading   = pcFontInst->GetLineGap();
			return( sHeight );
		}
	}
	font_height sHeight;
	sHeight.ascent  = 0;
	sHeight.descent = 0;
	sHeight.leading   = 0;
	return( sHeight );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetHighColor( int nRed, int nGreen, int nBlue, int nAlpha )
{
	fLayerData->highcolor.SetColor(nRed, nGreen, nBlue, nAlpha);
	m_bFontPalletteValid = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetHighColor( rgb_color sColor )
{
	fLayerData->highcolor.SetColor(sColor);
	m_bFontPalletteValid  = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetLowColor( int nRed, int nGreen, int nBlue, int nAlpha )
{
	fLayerData->lowcolor.SetColor(nRed, nGreen, nBlue, nAlpha);
	m_bFontPalletteValid = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetLowColor( rgb_color sColor )
{
	fLayerData->lowcolor.SetColor(sColor);
	m_bFontPalletteValid = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetEraseColor( int nRed, int nGreen, int nBlue, int nAlpha )
{
	fLayerData->viewcolor.SetColor(nRed, nGreen, nBlue, nAlpha);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::SetEraseColor( rgb_color sColor )
{
	fLayerData->viewcolor.SetColor(sColor);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Layer::RequestDraw(const IRect& cUpdateRect, bool bUpdate)
{
	if ( _hidden > 0 || fIsUpdating == true || NULL == m_pUserObject )
	{
		return;
	}

	BMessenger* pcTarget = fServerWin->GetAppTarget();

	if ( NULL != pcTarget )
	{
		BMessage cMsg( _UPDATE_ );

		cMsg.AddPointer( "_widget", GetUserObject() );
		cMsg.AddRect( "frame", (cUpdateRect - m_cIScrollOffset).AsBRect() );

		if ( pcTarget->SendMessage( &cMsg ) < 0 )
		{
			printf( "Layer::RequestDraw() failed to send _UPDATE_ message to %s\n", m_cName.c_str() );
		}
	}
}


/*!
	\brief Converts the rectangle to the layer's parent coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertToParent(BRect rect)
{
	return (rect.OffsetByCopy(fFrame.LeftTop()));
}

/*!
	\brief Converts the region to the layer's parent coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertToParent(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertToParent(reg->RectAt(i)));
	return BRegion(newreg);
}

/*!
	\brief Converts the rectangle from the layer's parent coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertFromParent(BRect rect)
{
	return (rect.OffsetByCopy(fFrame.left*-1,fFrame.top*-1));
}

/*!
	\brief Converts the region from the layer's parent coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertFromParent(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertFromParent(reg->RectAt(i)));
	return BRegion(newreg);
}

/*!
	\brief Converts the region to screen coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertToTop(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertToTop(reg->RectAt(i)));
	return BRegion(newreg);
}

/*!
	\brief Converts the rectangle to screen coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertToTop(BRect rect)
{
	if (fParent!=NULL)
		return(fParent->ConvertToTop(rect.OffsetByCopy(fFrame.LeftTop())) );
	else
		return(rect);
}

/*!
	\brief Converts the region from screen coordinates
	\param the region to convert
	\return the converted region
*/
BRegion Layer::ConvertFromTop(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertFromTop(reg->RectAt(i)));
	return BRegion(newreg);
}

/*!
	\brief Converts the rectangle from screen coordinates
	\param the rectangle to convert
	\return the converted rectangle
*/
BRect Layer::ConvertFromTop(BRect rect)
{
	if (fParent!=NULL)
		return(fParent->ConvertFromTop(rect.OffsetByCopy(fFrame.LeftTop().x*-1,
			fFrame.LeftTop().y*-1)) );
	else
		return(rect);
}
/*!
	\brief Makes the layer the backmost layer belonging to its parent
	
	This function will do nothing if the Layer has no parent or no siblings. Region 
	rebuilding is not performed.
*/
void Layer::MakeTopChild(void)
{
	// Handle redundant and pointless cases
	if(!fParent || (!fUpperSibling && !fLowerSibling) || fParent->fTopChild==this)
		return;

	Layer* pcParent = fParent;

	pcParent->RemoveChild( this );
	pcParent->AddChild( this, true );

	fParent->_regions_invalid = true;
	SetDirtyRegFlags();

	Layer* pcSibling;
	for ( pcSibling = fParent->fBottomChild ; pcSibling != NULL ; pcSibling = pcSibling->fUpperSibling )
	{
		if (pcSibling->fFrame.Contains(fFrame))
		{
			pcSibling->MarkModified(fFrame.OffsetByCopy(-pcSibling->fFrame.LeftTop()));
		}
	}
}

/*!
	\brief Makes the layer the frontmost layer belonging to its parent
	
	This function will do nothing if the Layer has no parent or no siblings. Region 
	rebuilding is not performed.
*/
void Layer::MakeBottomChild(void)
{
	// Handle redundant and pointless cases
	if(!fParent || (!fUpperSibling && !fLowerSibling) || fParent->fBottomChild==this)
		return;

	Layer* pcParent = fParent;

	pcParent->RemoveChild( this );
	pcParent->AddChild( this, false );

	fParent->_regions_invalid = true;
	SetDirtyRegFlags();

	Layer* pcSibling;
	for ( pcSibling = fParent->fBottomChild ; pcSibling != NULL ; pcSibling = pcSibling->fUpperSibling )
	{
		if (pcSibling->fFrame.Contains(fFrame))
		{
			pcSibling->MarkModified(fFrame.OffsetByCopy(-pcSibling->fFrame.LeftTop()));
		}
	}
}
/*
 @log
 	* added initialization in contructor to (0.0, 0.0) for fBoundsLeftTop member.
 	* modified Bounds() to use that member.
 	* some changes into RemoveChild()
*/


