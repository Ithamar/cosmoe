/*
 *  The Cosmoe application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2002 Bill Hayden
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef        __VESADRV_H__
#define        __VESADRV_H__

#include "BitmapDriver.h"
#include <vector>
#include <IRect.h>


class        SrvSprite;
class ServerBitmap;


struct VesaMode : public ScreenMode
{
			VesaMode( int w,
					  int h,
					  int bbl,
					  color_space cs,
					  int mode,
					  uint32 fb ) : ScreenMode( w,h,bbl,cs ) { m_nVesaMode = mode; m_nFrameBuffer = fb; }
	int		m_nVesaMode;
	uint32	m_nFrameBuffer;
};

class VesaDriver : public BitmapDriver
{
public:

							VesaDriver();
	virtual					~VesaDriver();

	area_id					Initialize();

	virtual int				GetScreenModeCount();
	virtual bool			GetScreenModeDesc( int nIndex, ScreenMode* psMode );

	int						SetScreenMode( int nWidth,
										int nHeight,
										color_space eColorSpc,
										int nPosH,
										int nPosV,
										int nSizeH,
										int nSizeV, float vRefreshRate );

	virtual int				GetHorizontalRes();
	virtual int				GetVerticalRes();
	virtual int				GetBytesPerLine();
	virtual color_space		GetColorSpace();
	virtual int				GetFramebufferOffset() { return( m_nFrameBufferOffset ); }

	void					SetColor( int nIndex, const rgb_color& sColor );

	virtual void			HideCursor(void);
	virtual void			MoveCursorTo(float x, float y);
	virtual void			SetCursor(ServerCursor *cursor);

	virtual void		OldStrokeLine( ServerBitmap* psBitMap, BRect clip,
										BPoint start, BPoint end,
										const rgb_color& color, int mode );
	virtual bool        OldFillRect( ServerBitmap* psBitMap, BRect r, const rgb_color& sColor );
	virtual bool        BltBitmap( ServerBitmap* pcDstBitMap, ServerBitmap* pcSrcBitMap, IRect cSrcRect, IPoint cDstPos, int nMode );
	static bool			ClipLine( const IRect& cRect, int* x1, int* y1, int* x2, int* y2 );

protected:
	void					FillBlit8 (uint8*  dst, int nMod, int W, int H, int color);
	void					FillBlit16(uint16* dst, int nMod, int W, int H, uint32 color);
	void					FillBlit24(uint8*  dst, int nMod, int W, int H, uint32 color);
	void					FillBlit32(uint32* dst, int nMod, int W, int H, uint32 color);

private:
	bool					InitModes();
	bool					SetVesaMode( uint32 nMode );

	IPoint					m_cMousePos;
	SrvSprite*				m_pcMouseSprite;
	int						m_nCurrentMode;
	uint32					m_nFrameBufferSize;
	int						m_nFrameBufferOffset;
	std::vector<VesaMode>	m_cModeList;

	ServerBitmap *_target;
};

#endif // __VESADRV_H__
