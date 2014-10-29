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
//	File Name:		SDLDriver.h
//	Author:			Bill Hayden <hayden@haydentech.com>
//	Description:	Display driver which renders to SDL
//
//------------------------------------------------------------------------------

#ifndef __SDLDRIVER_H__
#define __SDLDRIVER_H__

#include "BitmapDriver.h"
#include "SDL/SDL.h"

class BPortLink;

class SDLDriver : public BitmapDriver
{
public:
					SDLDriver();
	virtual			~SDLDriver();

	virtual bool	Initialize();
	virtual void	Shutdown();

	virtual void	InvertRect(const BRect &r);
	
	virtual uint32	GetQualifiers(void);

private:
	friend void		SDLEventTranslator(void *arg);

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
	void InvalidateSDL(const SDL_Rect &r);
	
	void DrawPixel(int x, int y, uint32 color);

	sem_id						drawsem;

	SDL_Surface*			mScreen;
	BPortLink*				serverlink;
};

#endif // __SDLDRIVER_H__
