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
//	File Name:		SDLDriver.cpp
//	Author:			Bill Hayden <hayden@haydentech.com>
//	Description:	Display driver which renders to SDL
//
//------------------------------------------------------------------------------


#include <pthread.h>

#include <SupportDefs.h>
#include <Bitmap.h>

#include "SDL/SDL.h"
#include "sdldriver.h"
#include "ServerBitmap.h"
#include "ServerConfig.h"
#include "RectUtils.h"

#include <PortLink.h>
#include <ServerProtocol.h>

#define DEBUG_SDL_DRIVER

#ifdef DEBUG_SDL_DRIVER
#include <stdio.h>
	#define STRACE(a) printf(a)
#else
	#define STRACE(x) ;
#endif


static void RectToSDLRect(const BRect& r, SDL_Rect& outRect);


/*!
	\brief Sets up internal variables needed by the SDLDriver
*/
SDLDriver::SDLDriver() : BitmapDriver()
{
	STRACE( "SDLDriver constructor\n" );

	mScreen = NULL;
	
	// This link for sending mouse messages to the AppServer.
	// This is only to take the place of the Input Server for testing purposes.
	// Obviously, Cosmoe does not intend to use X11 except for testing.
	serverlink = new BPortLink(find_port(SERVER_INPUT_PORT));

	drawsem = create_sem(1, "X11 draw semaphore");
}


SDLDriver::~SDLDriver()
{
	STRACE( "SDLDriver destructor\n" );

	delete serverlink;
}


/*!
	\brief Translate X11 events into appserver events, as if they came
			right from the actual Input Server
*/
void SDLEventTranslator(void *arg)
{
	SDLDriver *driver= (SDLDriver*)arg;
	SDL_Event event;
	int quit = 0;
	float x, y;

	/* Loop until an SDL_QUIT event is found */
	while( !quit )
	{
		/* Poll for events */
		while( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
				case SDL_MOUSEMOTION:
				{
					x=(float)event.motion.x;
					y=(float)event.motion.y;

					//STRACE("SDLDriver::MouseMoved\n");

					uint32 buttons = 0;
					int64 time = (int64)real_time_clock();

					driver->serverlink->StartMessage(B_MOUSE_MOVED);
					driver->serverlink->Attach<int64>(time);
					driver->serverlink->Attach<float>(x);
					driver->serverlink->Attach<float>(y);
					driver->serverlink->Attach<int32>(buttons);
					driver->serverlink->Flush();
					break;
				}

				case SDL_MOUSEBUTTONDOWN:{
					STRACE("MouseDown\n");
					uint32 buttons = event.button.button;
					uint32 clicks = 1;		// can't get the # of clicks without a *lot* of extra work :(
					uint32 mod = 0;
					int64 time = (int64)real_time_clock();

					driver->serverlink->StartMessage(B_MOUSE_DOWN);
					driver->serverlink->Attach<int64>(time);
					driver->serverlink->Attach<float>(x);
					driver->serverlink->Attach<float>(y);
					driver->serverlink->Attach<int32>(mod);
					driver->serverlink->Attach<int32>(buttons);
					driver->serverlink->Attach<int32>(clicks);
					driver->serverlink->Flush();
					break;
				}

				case SDL_MOUSEBUTTONUP:{
					STRACE("MouseUp\n");
					uint32 mod = 0;
					int64 time = (int64)real_time_clock();

					driver->serverlink->StartMessage(B_MOUSE_UP);
					driver->serverlink->Attach<int64>(time);
					driver->serverlink->Attach<float>(x);
					driver->serverlink->Attach<float>(y);
					driver->serverlink->Attach<int32>(mod);
					driver->serverlink->Flush();
					break;
				}

				/* Keyboard event */
				case SDL_KEYDOWN:
				{
					STRACE("KeyDown\n");

					int32 scancode, repeatcount,modifiers;
					modifiers = event.key.keysym.mod;
					int64 time = (int64)real_time_clock();
					
					repeatcount = 1;
					scancode = event.key.keysym.sym;
					driver->serverlink->StartMessage(B_KEY_DOWN);
					driver->serverlink->Attach<int64>(time);
					driver->serverlink->Attach<int32>(scancode);
					driver->serverlink->Attach<int32>(repeatcount);
					driver->serverlink->Attach<int32>(modifiers);
					//driver->serverlink->Attach(utf8data,sizeof(int8)*3);
					//driver->serverlink->Attach(keyarray,sizeof(int8)*16);
					driver->serverlink->Flush();
					
					/* the Escape quits Cosmoe, for now... */
					if(event.key.keysym.sym == SDLK_ESCAPE)
						quit = 1;
					break;
				}
				
				case SDL_KEYUP:
				{
					STRACE("KeyUp\n");

					int32 scancode, repeatcount,modifiers;
					modifiers = event.key.keysym.mod;
					repeatcount = 1;
					scancode = event.key.keysym.sym;
					int64 time = (int64)real_time_clock();
					
					driver->serverlink->StartMessage(B_KEY_DOWN);
					driver->serverlink->Attach<int64>(time);
					driver->serverlink->Attach<int32>(scancode);
					driver->serverlink->Attach<int32>(repeatcount);
					driver->serverlink->Attach<int32>(modifiers);
					//driver->serverlink->Attach(utf8data,sizeof(int8)*3);
					//driver->serverlink->Attach(keyarray,sizeof(int8)*16);
					driver->serverlink->Flush();

					break;
				}

				/* SDL_QUIT event (window close) */
				case SDL_QUIT:
					quit = 1;
					break;

				default:
					break;
			}
		}
	}

	BPortLink applink(find_port(SERVER_PORT_NAME));

	STRACE("SDLDriver: sending B_QUIT_REQUESTED message\n");
	applink.StartMessage(B_QUIT_REQUESTED);
	applink.Flush();

	STRACE("Leaving SDLEventTranslator\n");
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not

	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool SDLDriver::Initialize(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		return false;
	}

	mScreen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE);
	
	if (mScreen == NULL)
	{
		printf("Couldn't set 800x600x32 video mode: %s\n", SDL_GetError());
		return false;
	}

	// Create a new thread for mouse and key events
	pthread_t input_thread;
	pthread_create (&input_thread,
					NULL,
					(void *(*) (void *))&SDLEventTranslator,
					(void *) this);

	UtilityBitmap* target;
	target = new UtilityBitmap(BRect(0, 0, 799, 599), B_RGBA32, 0);
	target->_SetBuffer(mScreen->pixels);
	SetTarget(target);

	SDL_WM_SetCaption("Welcome to Cosmoe", NULL);
	
	SDL_ShowCursor(0);

	return true;
}

/*!
	\brief Shuts down the driver's video subsystem

	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void SDLDriver::Shutdown( void )
{
	STRACE( "SDLDriver::Close()\n" );
	SDL_Quit();
}


/*!
	\brief Inverts the colors in the rectangle.
	\param r Rectangle of the area to be inverted. Guaranteed to be within bounds.
*/
void SDLDriver::InvertRect(const BRect &r)
{
	STRACE("SDLDriver::InvertRect()\n");
	
	// Lock the screen, if necessary, so we can safely play
	// with the underlying pixels.
	if (SDL_MUSTLOCK(mScreen))
	{
		if (SDL_LockSurface(mScreen) < 0)
			return;
	}
	
	BitmapDriver::InvertRect(r);
	
	// Unlock the screen if we locked it
	if (SDL_MUSTLOCK(mScreen))
	{
		SDL_UnlockSurface(mScreen);
	}
	
	Invalidate(r);
}


// TODO: determine why we may need the DrawData parameter
void SDLDriver::Blit(const BRect &src, const BRect &dest, const DrawData *d)
{
	SDL_Rect source, destination;
	bool success;
	
	STRACE("SDLDriver::Blit()\n");
	
	RectToSDLRect(src, source);
	RectToSDLRect(dest, destination);
	
	success = (SDL_BlitSurface(mScreen, &source, mScreen, &destination) == 0);
	
	if (success)
		InvalidateSDL(destination);
}


void SDLDriver::FillSolidRect(const BRect &r, const RGBColor &color)
{
	STRACE("SDLDriver::FillSolidRect()\n");

	rgb_color col=color.GetColor32();
	Uint32	aColor = SDL_MapRGB(mScreen->format, col.red, col.green, col.blue);
	SDL_Rect aRect;
	bool success;

	RectToSDLRect(r, aRect);
	
	success = (SDL_FillRect(mScreen, &aRect, aColor) == 0);

	if (success)
		InvalidateSDL(aRect);
}


void SDLDriver::FillPatternRect(const BRect &r, const DrawData *d)
{
	STRACE("SDLDriver::FillPatternRect()\n");
	
	// Lock the screen, if necessary, so we can safely play
	// with the underlying pixels.
	if (SDL_MUSTLOCK(mScreen))
	{
		if (SDL_LockSurface(mScreen) < 0)
			return;
	}
	
	BitmapDriver::FillPatternRect(r, d);
	
	// Unlock the screen if we locked it
	if (SDL_MUSTLOCK(mScreen))
	{
		SDL_UnlockSurface(mScreen);
	}
	
	Invalidate(r);
}


void SDLDriver::StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color)
{
	STRACE("SDLDriver::StrokeSolidLine()\n");
	
	rgb_color col=color.GetColor32();
	Uint32	aColor = SDL_MapRGB(mScreen->format, col.red, col.green, col.blue);
	SDL_Rect aRect;
	bool success;
	
	// Do we need to bother with the vertical and horizontal cases?
	// I guess it boils down to this: does SDL accelerate rectangle
	// drawing?

	if (x1 == x2) /* vertical line */
	{
		aRect.w = 1;
		aRect.h = (max_c(y2, y1) - min_c(y2, y1)) + 1;
		aRect.x = x1;
		aRect.y = min_c(y1, y2);
		
		success = (SDL_FillRect(mScreen, &aRect, aColor) == 0);
	}
	else if (y1 == y2) /* horizontal line */
	{
		aRect.w = (max_c(x2, x1) - min_c(x2, x1)) + 1;
		aRect.h = 1;
		aRect.x = min_c(x1, x2);
		aRect.y = y1;
		
		success = (SDL_FillRect(mScreen, &aRect, aColor) == 0);
	}
	else
	{
		int dx = x2 - x1;
		int dy = y2 - y1;
		int steps;
		double xInc, yInc;
		double x = x1;
		double y = y1;
		
		aRect.w = (max_c(x2, x1) - min_c(x2, x1)) + 1;
		aRect.h = (max_c(y2, y1) - min_c(y2, y1)) + 1;
		aRect.x = min_c(x1, x2);
		aRect.y = min_c(y1, y2);
		
		if ( abs(dx) > abs(dy) )
			steps = abs(dx);
		else
			steps = abs(dy);
		xInc = dx / (double) steps;
		yInc = dy / (double) steps;
	
		// Lock the screen, if necessary, so we can safely play
		// with the underlying pixels.
		if (SDL_MUSTLOCK(mScreen))
		{
			if (SDL_LockSurface(mScreen) < 0)
				return;
		}
		
		DrawPixel((int)x, (int)y, aColor);
		for (int k=0; k<steps; k++)
		{
			x += xInc;
			y += yInc;
			DrawPixel((int)x, (int)y, aColor);
		}
		
		// Unlock the screen if we locked it
		if (SDL_MUSTLOCK(mScreen))
		{
			SDL_UnlockSurface(mScreen);
		}
		
		success = true;
	}

	if (success)
		InvalidateSDL(aRect);
}


void SDLDriver::StrokePatternLine(int32 x1, int32 y1, int32 x2, int32 y2, const DrawData *d)
{
	STRACE("SDLDriver::StrokePatternLine()\n");
	
	rgb_color col;
	Uint32	aColor;
	SDL_Rect aRect;
	
	int dx = x2 - x1;
	int dy = y2 - y1;
	int steps;
	double xInc, yInc;
	double x = x1;
	double y = y1;
	
	aRect.w = (max_c(x2, x1) - min_c(x2, x1)) + 1;
	aRect.h = (max_c(y2, y1) - min_c(y2, y1)) + 1;
	aRect.x = min_c(x1, x2);
	aRect.y = min_c(y1, y2);
	
	if ( abs(dx) > abs(dy) )
		steps = abs(dx);
	else
		steps = abs(dy);
	xInc = dx / (double) steps;
	yInc = dy / (double) steps;

	// Lock the screen, if necessary, so we can safely play
	// with the underlying pixels.
	if (SDL_MUSTLOCK(mScreen))
	{
		if (SDL_LockSurface(mScreen) < 0)
			return;
	}
	
	col = fDrawPattern.GetColor((int)x,(int)y).GetColor32();
	aColor = SDL_MapRGB(mScreen->format, col.red, col.green, col.blue);
	DrawPixel((int)x, (int)y, aColor);
	
	for (int k=0; k<steps; k++)
	{
		x += xInc;
		y += yInc;
		
		col = fDrawPattern.GetColor((int)x,(int)y).GetColor32();
		aColor = SDL_MapRGB(mScreen->format, col.red, col.green, col.blue);
		DrawPixel((int)x, (int)y, aColor);
	}
	
	// Unlock the screen if we locked it
	if (SDL_MUSTLOCK(mScreen))
	{
		SDL_UnlockSurface(mScreen);
	}
	
	InvalidateSDL(aRect);
}


void SDLDriver::StrokeSolidRect(const BRect &rect, const RGBColor &color)
{
	STRACE("SDLDriver::StrokeSolidRect()\n");
	
	StrokeSolidLine((int)rect.left, (int)rect.top, (int)rect.right, (int)rect.top, color);
	StrokeSolidLine((int)rect.left, (int)rect.bottom, (int)rect.right, (int)rect.bottom, color);
	StrokeSolidLine((int)rect.left, (int)rect.top, (int)rect.left, (int)rect.bottom, color);
	StrokeSolidLine((int)rect.right, (int)rect.top, (int)rect.right, (int)rect.bottom, color);
}


void SDLDriver::CopyBitmap(ServerBitmap *bitmap, const BRect &sourcerect, const BRect &dest, const DrawData *d)
{
	STRACE("SDLDriver::CopyBitmap()\n");

	// Lock the screen, if necessary, so we can safely play
	// with the underlying pixels.
	if (SDL_MUSTLOCK(mScreen))
	{
		if (SDL_LockSurface(mScreen) < 0)
			return;
	}
	
	BitmapDriver::CopyBitmap(bitmap, sourcerect, dest, d);
	
	// Unlock the screen if we locked it
	if (SDL_MUSTLOCK(mScreen))
	{
		SDL_UnlockSurface(mScreen);
	}
	
	BRect destrect(sourcerect);
	destrect.OffsetTo(dest.left, dest.top);
	Invalidate(destrect);
}


/*!
	\brief Refresh the SDL bitmap with the contents of the ServerBitmap
	\param r      The BRect rectangle to refresh
*/
void SDLDriver::Invalidate(const BRect &r)
{
	SDL_Rect aRect;
	BRect damage(r & fTarget->Bounds());
	RectToSDLRect(damage, aRect);

	InvalidateSDL(aRect);
}


/*!
	\brief Refresh the SDL bitmap with the contents of the ServerBitmap
	\param r      The BRect rectangle to refresh
*/
void SDLDriver::InvalidateSDL(const SDL_Rect &r)
{
	acquire_sem(drawsem);
	SDL_UpdateRect(mScreen, r.x, r.y, r.w, r.h);
	release_sem(drawsem);
}


/*!
	\brief Convert a BRect to an SDL_Rect
	\param r        The source BRect
	\param outRect  The SDL_Rect to make equal to the BRect
*/
static void RectToSDLRect(const BRect& r, SDL_Rect& outRect)
{
	outRect.w = r.IntegerWidth() + 1;
	outRect.h = r.IntegerHeight() + 1;
	outRect.x = (int)r.left;
	outRect.y = (int)r.top;
}


/*!
	\brief Set a single pixel to "color"
	\param x      The x-coordinate of the pixel to set
	\param y      The y-coordinate of the pixel to set
	\param color  The color in SDL Uint32 format
	\note         The SDL lock must be held when you call this
*/
void SDLDriver::DrawPixel(int x, int y, uint32 color)
{  
	switch (mScreen->format->BytesPerPixel)
	{
		case 1: // 8-bpp
		{
			Uint8 *bufp;
			bufp = (Uint8 *)mScreen->pixels + y*mScreen->pitch + x;
			*bufp = color;
		}
		break;
		
		case 2: // 15-bpp or 16-bpp
		{
			Uint16 *bufp;
			bufp = (Uint16 *)mScreen->pixels + y*mScreen->pitch/2 + x;
			*bufp = color;
		}
		break;
		
		case 3: // 24-bpp
		{
			Uint8 *bufp;
			bufp = (Uint8 *)mScreen->pixels + y*mScreen->pitch + x * 3;
			if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
			{
				bufp[0] = color;
				bufp[1] = color >> 8;
				bufp[2] = color >> 16;
			}
			else
			{
				bufp[2] = color;
				bufp[1] = color >> 8;
				bufp[0] = color >> 16;
			}
		}
		break;
		
		case 4: // 32-bpp
		{
			Uint32 *bufp;
			bufp = (Uint32 *)mScreen->pixels + y*mScreen->pitch/4 + x;
			*bufp = color;
		}
		break;
	}
}
