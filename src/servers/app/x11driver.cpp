//------------------------------------------------------------------------------
//	Copyright (c) 2004, Bill Hayden
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
//	File Name:		X11Driver.cpp
//	Author:			Bill Hayden <hayden@haydentech.com>
//	Description:	Display driver which renders into an X11 window
//
//------------------------------------------------------------------------------

#include <string.h>
#include <stdio.h>

#include <SupportDefs.h>
#include <InterfaceDefs.h>

#include <kernel.h>
#include <ServerFont.h>
#include "ServerWindow.h"
#include <Bitmap.h>

#include "x11driver.h"
#include "ServerBitmap.h"
#include "RectUtils.h"

#include <ServerCursor.h>
#include "AppServer.h" 

//#define DEBUG_X11_DRIVER

#ifdef DEBUG_X11_DRIVER
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif


#define X11DRIVER_WIDTH		1024
#define X11DRIVER_HEIGHT	768
#define X11DRIVER_DEPTH		16

inline int MIN_INT(int a,int b)
{
	if(a<b)
		return a;
	return b;
}

inline int MAX_INT(int a,int b)
{
	if(b<a)
		return a;
	return b;
}

using namespace X11;

/*!
	\brief Sets up internal variables needed by the X11Driver
*/
X11Driver::X11Driver(void) : BitmapDriver()
{
	mysem= create_sem(1, "prevent X crash");

	STRACE("X11Driver::X11Driver\n");

	drawsem = create_sem(1, "X11 draw semaphore");
}


X11Driver::~X11Driver()
{
	STRACE("X11Driver::~X11Driver\n");
}

/*!
	\brief Translate X11 events into appserver events, as if they came
			right from the actual Input Server
*/
void XEventTranslator(void *arg)
{
	X11Driver *driver= (X11Driver*)arg;
	XEvent x_event;
	int quit = 0;

	STRACE("X11Driver::XEventTranslator\n");

	while( !quit )
	{
		XNextEvent(driver->display, &x_event);
		acquire_sem(driver->mysem);
		switch(x_event.type)
		{
			case MotionNotify:
			{
				int x=((XMotionEvent *) &x_event)->x;
				int y=((XMotionEvent *) &x_event)->y;
				//printf("MouseMoved (%d,%d)\n",x,y);
				//InputNode::SetMousePos( BPoint(x,y) );
				BMessage* pcEvent = new BMessage( B_MOUSE_MOVED );
				pcEvent->AddPoint( "delta_move", BPoint(x,y) - AppServer::s_cMousePos );
				app_server->EnqueueEvent(pcEvent, driver->GetQualifiers());
				break;
			}

			case Expose:{
				STRACE("Draw\n");
				if (driver->fTarget != NULL)
					driver->Invalidate(BRect(0, 0, 1024, 768));
				break;
			}

			case KeyPress:{
				STRACE("KeyDown\n");
				
				bool shift = x_event.xkey.state & ShiftMask;
				bool control = x_event.xkey.state & ControlMask;
				
				driver->SetQualifiers( (B_LEFT_SHIFT_KEY * shift) | 
									   (B_LEFT_CONTROL_KEY * control) );
				char k=XLookupKeysym(&x_event.xkey, 0);
				char* key=(char*)malloc(2*sizeof(char));
				key[0]=k;
				key[1]=0;
				// hack :)
				ServerWindow::SendKbdEvent(((XKeyEvent *) &x_event)->keycode,
											driver->GetQualifiers(),
											key,
											key);
				break;
			}

			case KeyRelease:{
				STRACE("KeyUp\n");
				bool shift = x_event.xkey.state & ShiftMask;
				bool control = x_event.xkey.state & ControlMask;
				
				driver->SetQualifiers( (B_LEFT_SHIFT_KEY * shift) | 
									   (B_LEFT_CONTROL_KEY * control) );
				break;
			}

			case ButtonPress:{
				STRACE("MouseDown\n");
				BMessage*  pcEvent = new BMessage(B_MOUSE_DOWN);
				pcEvent->AddInt32( "buttons",  ((XButtonEvent *) &x_event)->button);
				app_server->EnqueueEvent(pcEvent, driver->GetQualifiers());
				break;
			}

			case ButtonRelease:{
				//STRACE("MouseUp\n");
				BMessage*  pcEvent = new BMessage(B_MOUSE_UP);
				pcEvent->AddInt32( "buttons", ((XButtonEvent *) &x_event)->button );
				app_server->EnqueueEvent(pcEvent, driver->GetQualifiers());
				break;
			}
			
			default:
				break;
		}

		XFlush(driver->display);
		release_sem(driver->mysem);
	}

	STRACE("Leaving X11EventTranslator\n");
}


/*!
	\brief Initializes the driver object.
	\return true if successful, false if not

	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool X11Driver::Initialize(void)
{
	STRACE("X11Driver::Initialize\n");

	display = XOpenDisplay(NULL);
	STRACE("XOpenDisplay obtained display\n");

	screen = DefaultScreen(display);
	depth = DefaultDepth(display, screen);
	window_attributes.border_pixel = BlackPixel(display, screen);
	window_attributes.background_pixel = WhitePixel(display, screen);
	window_attributes.override_redirect = True;
	window_mask = CWBackPixel | CWBorderPixel;

	xcanvas = XCreateWindow(display, DefaultRootWindow(display),
				0, 0, X11DRIVER_WIDTH, X11DRIVER_HEIGHT, 0, depth,
				InputOutput, CopyFromParent,
				CWBackPixel | CWBorderPixel /*if you need noborder */
				/*| CWOverrideRedirect*/, &window_attributes);
	image_gc = XCreateGC(display, xcanvas, 0, 0);
	XMapWindow(display, xcanvas);
	XFlush(display);
	XSelectInput(display, xcanvas, ExposureMask | PointerMotionMask |
			KeyPressMask | ButtonPressMask  | ButtonReleaseMask | StructureNotifyMask);
	STRACE("passed XCreateWindow\n");

	// Create a new thread for mouse and key events
	pthread_t input_thread;
	pthread_create (&input_thread,
					NULL,
					(void *(*) (void *))&XEventTranslator,
					(void *) this);

	UtilityBitmap* target;
	target = new UtilityBitmap(BRect(0, 0,
								X11DRIVER_WIDTH - 1, X11DRIVER_HEIGHT - 1),
								B_RGB32, 0);
	target->m_pcDriver = this;
	SetTarget(target);
	STRACE("passed SetTarget()\n");

	// Then we point XCreateImage at the UtilityBitmap's bits
	ximage = XCreateImage (display, CopyFromParent, depth, ZPixmap, 0,
						(char*)fTarget->Bits(), X11DRIVER_WIDTH, X11DRIVER_HEIGHT,
						X11DRIVER_DEPTH, X11DRIVER_WIDTH * 4);


	xpixmap = XCreatePixmap(display, xcanvas,
								X11DRIVER_WIDTH, X11DRIVER_HEIGHT, depth);
	XPutImage(display, xpixmap, image_gc, ximage, 0, 0, 0, 0,
				X11DRIVER_WIDTH, X11DRIVER_HEIGHT);
	XFlush(display);
		
	return true;
}


void X11Driver::InvertRect(const BRect &r)
{
	STRACE("X11Driver::InvertRect()\n");

	BitmapDriver::InvertRect(r);

	Invalidate(r);
}


void X11Driver::Blit(const BRect &src, const BRect &dest, const DrawData *d)
{
	STRACE("X11Driver::Blit()\n");

	BitmapDriver::Blit(src, dest, d);

	Invalidate(dest);
}

void X11Driver::FillSolidRect(const BRect &r, const RGBColor &color)
{
	STRACE("X11Driver::FillSolidRect()\n");

	BitmapDriver::FillSolidRect(r, color);

	Invalidate(r);
}

void X11Driver::FillPatternRect(const BRect &r, const DrawData *d)
{
	STRACE("X11Driver::FillPatternRect()\n");

	BitmapDriver::FillPatternRect(r, d);

	Invalidate(r);
}

void X11Driver::StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color)
{
	STRACE("X11Driver::StrokeSolidLine()\n");
	
	BRect r(x1, y1, x2, y2);
	ValidateRect(&r);

	int dx = x2 - x1;
	int dy = y2 - y1;
	int steps;
	double xInc, yInc;
	double x = x1;
	double y = y1;
	
	if ( abs(dx) > abs(dy) )
		steps = abs(dx);
	else
		steps = abs(dy);
	xInc = dx / (double) steps;
	yInc = dy / (double) steps;

	DrawPixel((int)x, (int)y, color);
	for (int k=0; k<steps; k++)
	{
		x += xInc;
		y += yInc;
		DrawPixel((int)x, (int)y, color);
	}

	Invalidate(r);
}

void X11Driver::StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d)
{
	STRACE("X11Driver::StrokePatternLine()\n");

	// StrokePatternLine unimplemented, use StrokeSolidLine for now
#if 0
	BRect r(x1, y1, x2, y2);
	ValidateRect(&r);

	BitmapDriver::StrokePatternLine(x1, y1, x2, y2, d);

	Invalidate(r);
#else
	StrokeSolidLine(x1, y1, x2, y2, d->highcolor);
#endif
}

void X11Driver::StrokeSolidRect(const BRect &rect, const RGBColor &color)
{
	STRACE("X11Driver::StrokeSolidRect()\n");

	BitmapDriver::StrokeSolidLine(rect.left, rect.top, rect.right, rect.top, color);
	BitmapDriver::StrokeSolidLine(rect.right, rect.top, rect.right, rect.bottom, color);
	BitmapDriver::StrokeSolidLine(rect.right, rect.bottom, rect.left, rect.bottom, color);
	BitmapDriver::StrokeSolidLine(rect.left, rect.bottom, rect.left, rect.top, color);

	Invalidate(rect);
}


void X11Driver::CopyBitmap(ServerBitmap *bitmap, const BRect &sourcerect, const BRect &dest, const DrawData *d)
{
	STRACE("X11Driver::CopyBitmap()\n");

	BitmapDriver::CopyBitmap(bitmap, sourcerect, dest, d);
	
	BRect destrect(sourcerect);
	destrect.OffsetTo(dest.left, dest.top);
	Invalidate(destrect);
}


bool X11Driver::RenderGlyph( ServerBitmap *pcBitmap, Glyph* pcGlyph,
			    const IPoint& cPos, const IRect& cClipRect, const rgb_color& sFgColor )
{
	acquire_sem(mysem);


	IRect        cBounds        = pcGlyph->m_cBounds + cPos;
	IRect        cRect         = cBounds & cClipRect;

	if ( cRect.IsValid() == false )
	{
		release_sem(mysem);
		return true;
	}


	int sx = cRect.left - cBounds.left;
	int sy = cRect.top - cBounds.top;

	int nWidth  = cRect.Width()+1;
	int nHeight = cRect.Height()+1;

	int        nSrcModulo = pcGlyph->m_nBytesPerLine - nWidth;

	uint8*  pSrc = pcGlyph->m_pRaster + sx + sy * pcGlyph->m_nBytesPerLine;

	rgb_color        sCurCol;
	rgb_color        sBgColor;

	if ( pcBitmap->ColorSpace() == B_RGB15 )
	{
		int     nDstModulo = pcBitmap->BytesPerRow() / 2 - nWidth;
		uint16* pDst = (uint16*)pcBitmap->Bits() + cRect.left + (cRect.top * pcBitmap->BytesPerRow() / 2);

		int nFgClut = COL_TO_RGB15( sFgColor );

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;

				if ( nAlpha > 0 )
				{
					if ( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						*pDst = nFgClut;
					}
					else
					{
						int        nClut = *pDst;

						sBgColor = RGB16_TO_COL( nClut );


						sCurCol.red   = sBgColor.red   + (sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.green = sBgColor.green + (sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.blue  = sBgColor.blue  + (sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);

						*pDst = COL_TO_RGB15( sCurCol );
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	else if ( pcBitmap->ColorSpace() == B_RGB16 )
	{
		int        nDstModulo = pcBitmap->BytesPerRow() / 2 - nWidth;
		uint16* pDst = (uint16*)pcBitmap->Bits() + cRect.left + cRect.top * pcBitmap->BytesPerRow() / 2;

		int nFgClut = COL_TO_RGB16( sFgColor );

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;

				if ( nAlpha > 0 )
				{
					if ( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						*pDst = nFgClut;
					}
					else
					{
						int        nClut = *pDst;

						sBgColor = RGB16_TO_COL( nClut );

						sCurCol.red   = sBgColor.red   + int(sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.green = sBgColor.green + int(sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.blue  = sBgColor.blue  + int(sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);

						*pDst = COL_TO_RGB16( sCurCol );
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}
	else if ( pcBitmap->ColorSpace() == B_RGB32 )
	{
		int        nDstModulo = pcBitmap->BytesPerRow() / 4 - nWidth;
		uint32* pDst = (uint32*)pcBitmap->Bits() + cRect.left + cRect.top * pcBitmap->BytesPerRow() / 4;

		int nFgClut = COL_TO_RGB32( sFgColor );

		for ( int y = 0 ; y < nHeight ; ++y )
		{
			for ( int x = 0 ; x < nWidth ; ++x )
			{
				int nAlpha = *pSrc++;

				if ( nAlpha > 0 )
				{
					if ( nAlpha == NUM_FONT_GRAYS - 1 )
					{
						*pDst = nFgClut;
					}
					else
					{
						int        nClut = *pDst;

						sBgColor = RGB32_TO_COL( nClut );

						sCurCol.red   = sBgColor.red   + (sFgColor.red   - sBgColor.red)   * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.green = sBgColor.green + (sFgColor.green - sBgColor.green) * nAlpha / (NUM_FONT_GRAYS-1);
						sCurCol.blue  = sBgColor.blue  + (sFgColor.blue  - sBgColor.blue)  * nAlpha / (NUM_FONT_GRAYS-1);

						*pDst = COL_TO_RGB32( sCurCol );
					}
				}
				pDst++;
			}
			pSrc += nSrcModulo;
			pDst += nDstModulo;
		}
	}

	Invalidate(cRect.AsBRect());

	release_sem(mysem);
	return(true);
}


bool X11Driver::RenderGlyph( ServerBitmap *pcBitmap, Glyph* pcGlyph,
                                 const IPoint& cPos, const IRect& cClipRect, uint32* anPalette )
{
	rgb_color color = {0, 0, 0, 0}; 
	RenderGlyph(pcBitmap, pcGlyph,cPos, cClipRect, color);

	return(true);
}

/*!
	\brief Refresh the X11 window with the contents of the ServerBitmap
	\param r      The BRect rectangle to refresh
*/
void X11Driver::Invalidate(const BRect &r)
{
	BRect damage = (r & fTarget->Bounds());

	STRACE("X11Driver::Invalidate()\n");
	
	acquire_sem(drawsem);
	XPutImage(display, xcanvas, image_gc, ximage, damage.left, damage.top,
												  damage.left, damage.top,
												  damage.IntegerWidth() + 1,
												  damage.IntegerHeight() + 1);
	release_sem(drawsem);
}

/*!
	\brief Set a single pixel to "color"
	\param x      The x-coordinate of the pixel to set
	\param y      The y-coordinate of the pixel to set
	\param color  The color in SDL Uint32 format
	\note         The SDL lock must be held when you call this
*/
void X11Driver::DrawPixel(int x, int y, const RGBColor &color)
{
	int bytes_per_row = fTarget->BytesPerRow();
	RGBColor col(color);	// to avoid GetColor8/15/16() const issues

	switch(fTarget->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)fTarget->Bits() + y*bytes_per_row;
				uint8 color8 = col.GetColor8();
				fb[x] = color8;
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + y*bytes_per_row);
				uint16 color15 = col.GetColor15();
				fb[x] = color15;
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)fTarget->Bits() + y*bytes_per_row);
				uint16 color16 = col.GetColor16();
				fb[x] = color16;
			} break;
		case 24:
		case 32:
			{
				uint32 *fb = (uint32 *)((uint8 *)fTarget->Bits() + y*bytes_per_row);
				rgb_color fill_color = color.GetColor32();
				uint32 color32 = (fill_color.alpha << 24) | (fill_color.red << 16) | (fill_color.green << 8) | (fill_color.blue);
				fb[x] = color32;
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}

