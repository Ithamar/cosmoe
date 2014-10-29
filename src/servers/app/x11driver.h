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
//	File Name:		X11Driver.h
//	Author:			Bill Hayden <hayden@haydentech.com>
//	Description:	Display driver which renders into an X11 window
//
//------------------------------------------------------------------------------

#ifndef __X11DRIVER_H__
#define __X11DRIVER_H__

#include <stdlib.h>

#include "BitmapDriver.h"


namespace X11 {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#undef ScreenCount
};

class PortLink;

class X11Driver : public BitmapDriver
{
public:
					X11Driver();
	virtual			~X11Driver();

	bool			Initialize();

	bool        RenderGlyph( ServerBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos,
				const IRect& cClipRect, uint32* anPalette );
	bool        RenderGlyph( ServerBitmap* pcBitmap, Glyph* pcGlyph, const IPoint& cPos,
				const IRect& cClipRect, const rgb_color& sFgColor );

	virtual void	InvertRect(const BRect &r);

private:
	friend void		XEventTranslator(void *arg);

	virtual void Blit(const BRect &src, const BRect &dest, const DrawData *d);
	virtual void FillSolidRect(const BRect &rect, const RGBColor &color);
	virtual void FillPatternRect(const BRect &rect, const DrawData *d);
	virtual void StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color);
	virtual void StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d);
	virtual void StrokeSolidRect(const BRect &rect, const RGBColor &color);
	virtual void CopyBitmap(ServerBitmap *bitmap, const BRect &source, const BRect &dest, const DrawData *d);

	// This is for drivers which are internally double buffered and calling this will cause the real
	// framebuffer to be updated
	virtual void Invalidate(const BRect &r);
	void DrawPixel(int x, int y, const RGBColor &color);

	int							screen;
	int							depth;
	unsigned long				window_mask;

	X11::Display*				display;
	X11::Window					xcanvas;
	X11::XImage*				image;
	X11::Colormap				colormap;
	X11::GC						image_gc;
	X11::XImage*				ximage;
	X11::XSetWindowAttributes	window_attributes;
	X11::XSizeHints				window_hints;
	X11::Pixmap					xpixmap;

	sem_id						drawsem;

	sem_id						mysem;
};

#endif // __X11DRIVER_H__
