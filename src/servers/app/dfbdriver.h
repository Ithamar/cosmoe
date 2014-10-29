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
//	File Name:		dfbriver.h
//	Author:			Bill Hayden <hayden@haydentech.com>
//	Description:	Display driver which renders to DirectFB
//
//------------------------------------------------------------------------------

#ifndef        __DFBDRIVER_H__
#define        __DFBDRIVER_H__

#include "DisplayDriver.h"

#include <directfb.h>

extern IDirectFB*	DFBObject;

class DirectFBDriver : public DisplayDriver
{
public:

							DirectFBDriver();
	virtual					~DirectFBDriver();

	// Virtual methods which need to be implemented by each subclass
	virtual bool Initialize(void);
	virtual void Shutdown(void);

private:
	friend void		DFBEventTranslator(void *arg);

	// Two functions for gaining direct access to the framebuffer of a child class. This removes the need
	// for a set of glyph-blitting virtual functions for each driver.
	virtual bool AcquireBuffer(FBBitmap *bmp);
	virtual void ReleaseBuffer(void);

	virtual void FillSolidRect(const BRect &rect, const RGBColor &color);
	virtual void StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color);
	virtual void StrokeSolidRect(const BRect &rect, const RGBColor &color);

	// This is for drivers which are internally double buffered and calling this will cause the real
	// framebuffer to be updated
	virtual void Invalidate(const BRect &r);
	
	IDirectFB*				mDFBObject;
	IDirectFBSurface*		mPrimarySurface;
	IDirectFBDisplayLayer*	mLayer;
	IDirectFBEventBuffer*	mEventBuffer;
};

#endif
