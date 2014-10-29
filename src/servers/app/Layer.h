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
//	File Name:		Layer.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
#ifndef _LAYER_H_
#define _LAYER_H_


#include <SupportDefs.h>

#include <util/resource.h>
#include <GraphicsDefs.h>
#include <Rect.h>
#include <IRect.h>
#include <Region.h>
#include <List.h>
#include <String.h>
#include <OS.h>
#include <Locker.h>
#include <View.h>
#include <Font.h>
#include <string>
#include "LayerData.h"

#include "fontnode.h"

class Gate;
struct GRndCopyRect_s;

class FontNode;
class ServerBitmap;
class ServerWindow;
class ServerCursor;
class DisplayDriver;


rgb_color GetDefaultColor( int nIndex );

extern Gate g_cLayerGate;

#define NUM_FONT_GRAYS 256

/*!
	\class Layer Layer.h
	\brief Shadow BView class
	
	Layers provide all sorts of functionality. They are the shadow class for BViews, 
	but they also provide the base class for other classes which handle drawing to 
	the frame buffer, like RootLayer and WinBorder.
*/
class Layer
{
public:
	Layer(BRect frame, const char *name, Layer* pcParent,
			int32 flags, void* pUserObj, ServerWindow *win);
	Layer( ServerBitmap* pcBitmap, bool rootLayer = false );
	virtual ~Layer(void);

	void AddChild(Layer *child, bool bTopmost);
	void RemoveChild(Layer *child, bool rebuild=true);
	void RemoveSelf(bool rebuild=true);

	uint32 CountChildren(void) const;

	const char *GetName(void) const { return( m_cName.c_str() ); }

	void	Init();



	int				GetHandle( void ) const           { return( m_hHandle );         }
	void*			GetUserObject( void ) const       { return( m_pUserObject );     }
	void			SetUserObject( void* pObj )       { m_pUserObject = pObj;        }
	void			SetWindow( ServerWindow* pcWindow );
	ServerWindow*	GetWindow( void ) const           { return( fServerWin );        }

	bool                IsVisible() const                 { return( _hidden == 0 ); }
			void				MakeTopChild(void);
			void				MakeBottomChild(void);
			Layer*				GetUpperSibling() const { return fUpperSibling; }
			Layer*				GetLowerSibling() const { return fLowerSibling; }
			LayerData*			GetLayerData(void) const { return fLayerData; }
	void                Added( int nHideCount );
	int                 GetLevel() const                  { return( fLevel );          }
	Layer *GetChildAt(BPoint pt, bool recursive=false);
	Layer*              GetBottomChild( void ) const      { return( fBottomChild );   }
	Layer*              GetTopChild( void ) const         { return( fTopChild );      }

	Layer*              GetParent( void ) const           { return( fParent );        }

	void                SetBitmap( ServerBitmap* pcBitmap );
	ServerBitmap*          GetBitmap( void ) const           { return( m_pcBitmap );        }

	BRegion*        GetRegion();
	bool                IsBackdrop() const                { return( m_bBackdrop );       }
	int                 ToggleDepth();

	virtual void        SetFrame( const BRect& cRect );

	void                ScrollBy( const BPoint& cOffset );

	BPoint          GetLeftTop( void ) const          { return( BPoint( fFrame.left, fFrame.top ) );   }
	IPoint          GetILeftTop( void ) const         { return( IPoint( m_cIFrame.left, m_cIFrame.top ) ); }


	virtual void RequestDraw(const IRect &r, bool bUpdate = false);
	void Invalidate(const BRect &rect);
	void Invalidate(bool bRecursive = false);
	void RebuildRegions(bool bForce);
	bool IsDirty(void) const;
	void UpdateIfNeeded(bool force_update=false);
	void MarkModified(BRect rect);
	void UpdateRegions(bool force=true, bool bRoot=true);

	void Show(void);
	void Hide(void);
	bool IsHidden(void) const;

	BRect Bounds(void) const;
	BRect Frame(void) const;

	BRect ConvertToParent(BRect rect);
	BRegion ConvertToParent(BRegion *reg);
	BRect ConvertFromParent(BRect rect);
	BRegion ConvertFromParent(BRegion *reg);
	BRegion ConvertToTop(BRegion *reg);
	BRect ConvertToTop(BRect rect);
	BRegion ConvertFromTop(BRegion *reg);
	BRect ConvertFromTop(BRect rect);
	
	// Coordinate conversions:
	BPoint          ConvertToParent( const BPoint& cPoint ) const;
	void                ConvertToParent( BPoint* pcPoint ) const;
	void                ConvertToParent( BRect* pcRect ) const;
	BPoint          ConvertFromParent( const BPoint& cPoint ) const;
	void                ConvertFromParent( BPoint* pcPoint ) const;
	void                ConvertFromParent( BRect* pcRect ) const;
	BPoint          ConvertToRoot( const BPoint& cPoint )        const;
	void                ConvertToRoot( BPoint* pcPoint ) const;
	void                ConvertToRoot( BRect* pcRect ) const;
	BPoint          ConvertFromRoot( const BPoint& cPoint ) const;
	void                ConvertFromRoot( BPoint* pcPoint ) const;
	void                ConvertFromRoot( BRect* pcRect ) const;

	DisplayDriver *GetDisplayDriver(void) const { return fDriver; }
	ServerWindow *Window(void) const { return fServerWin; }

	void PruneTree(void);
	
	void PrintToStream(void);
	void PrintNode(void);
	void PrintTree(void);

	void                SetDirtyRegFlags();

	void                DeleteRegions();

	void                BeginUpdate( void );
	void                EndUpdate( void );

	void                SetFont( FontNode* pcFont );
	font_height     GetFontHeight() const;
	void                SetHighColor( int nRed, int nGreen, int nBlue, int nAlpha );
	void                SetHighColor( rgb_color sColor );
	void                SetLowColor( int nRed, int nGreen, int nBlue, int nAlpha );
	void                SetLowColor( rgb_color sColor );
	void                SetEraseColor( int nRed, int nGreen, int nBlue, int nAlpha );
	void                SetEraseColor( rgb_color sColor );
	void                DrawFrame( const BRect& cRect, uint32 nStyle );
	void                MovePenTo( float x, float y )       { fLayerData->penlocation.x = x; fLayerData->penlocation.y = y;  }
	void                MovePenTo( const BPoint& cPos ) { fLayerData->penlocation = cPos;  }
	void                MovePenBy( const BPoint& cPos ) { fLayerData->penlocation += cPos; }
	BPoint          GetPenPosition() const              { return( fLayerData->penlocation ); }
	void                DrawLine( const BPoint& cFromPnt, const BPoint& cToPnt ) { fLayerData->penlocation = cFromPnt; DrawLine( cToPnt ); }
	void                DrawLine( const BPoint& cToPnt );

	void                DrawString( const char* pzString, int nLength );

	void                CopyRect( ServerBitmap* pcBitmap, GRndCopyRect_s* psCmd );

	void                FillArc(BRect cRect, float start, float angle, rgb_color sColor);
	void                StrokeArc(BRect cRect, float start, float angle, rgb_color sColor);
	
	void                FillEllipse(BRect cRect, rgb_color sColor);
	void                StrokeEllipse(BRect cRect, rgb_color sColor);
	
	void                StrokeRect( BRect cRect, rgb_color sColor );
	void                FillRect( BRect cRect );
	void                FillRect( BRect cRect, rgb_color sColor );
	void                EraseRect( BRect cRect );
	void                DrawBitMap( ServerBitmap* pcDstBitmap, ServerBitmap* pcSrcBitmap, BRect cSrcRect, BPoint cDstPos );
	void                ScrollRect( ServerBitmap* pcBitmap, BRect cSrcRect, BPoint cDstPos );

	BPoint          GetScrollOffset() const  { return( m_cScrollOffset );  }
	IPoint          GetIScrollOffset() const { return( m_cIScrollOffset ); }

private:
	void ShowHideHelper(bool show);

	void            InvalidateNewAreas( void );
	void            ClearDirtyRegFlags();
	void            SwapRegions( bool bForce );
	void            MoveChildren();

protected:
	friend class WinBorder;
	friend class ServerWindow;

	int				m_hHandle;
	std::string     m_cName;
	void*           m_pUserObject;        // Pointer to our partner in the application (in the apps address space)
	ServerBitmap*      m_pcBitmap;           // The bitmap we are supposed to render to

	IRect           m_cIFrame;            // Frame rectangle relative to our parent
	BPoint          m_cScrollOffset;
	IPoint          m_cIScrollOffset;

	BRect fFrame;
	BPoint fBoundsLeftTop;
	Layer *fParent;
	Layer *fUpperSibling;
	Layer *fLowerSibling;
	Layer *fTopChild;
	Layer *fBottomChild;

	BRegion*        m_pcPrevVisibleReg;   // Temporary storage for m_pcVisibleReg during region rebuild
	BRegion*        m_pcPrevFullReg;      // Temporary storage for m_pcFullReg during region rebuild
	BRegion*        m_pcDrawReg;          // Only valid between BeginPaint()/EndPaint()
	BRegion*        m_pcActiveDamageReg;

	BRegion		*_visible,
				*_invalid,
				*_full;

	ServerWindow *fServerWin;
	BString *fName;	
	int32 fViewToken;
	int32 fLevel;
	int32 _flags;
	uint8 _hidden;
	bool		_is_dirty;
	bool fIsUpdating;
	bool		_regions_invalid;

	ServerCursor		*_cursor;
	DisplayDriver *fDriver;
	LayerData *fLayerData;
	//RGBColor fBackColor;

	IPoint          m_cDeltaMove;         // Relative movement since last region update
	IPoint          m_cDeltaSize;         // Relative sizing since last region update

	bool            m_bBackdrop;



	// Render state:
	rgb_color       m_asFontPallette[NUM_FONT_GRAYS];
	int             m_nRegionUpdateCount;
	int             m_nMouseOffCnt;
	FontNode*       m_pcFont;
	bool            m_bIsMouseOn;
	bool            m_bFontPalletteValid;
	bool            m_bIsAddedToFont;
	bool            isRootLayer;
	FontNode::DependencyList_t::iterator  m_cFontViewListIterator;
};

Layer* FindLayer( int32 nToken );

#endif

