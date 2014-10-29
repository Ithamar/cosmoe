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
#include <pthread.h>

#include <SupportDefs.h>

#include <Bitmap.h>

#include "x11driver.h"
#include "ServerBitmap.h"
#include "ServerConfig.h"
#include "RectUtils.h"

#include <PortLink.h>
#include <ServerProtocol.h>

#define DEBUG_X11_DRIVER

#ifdef DEBUG_X11_DRIVER
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif

// Hardcode 800x600x32 for now
#define X11DRIVER_WIDTH		800
#define X11DRIVER_HEIGHT	600
#define X11DRIVER_DEPTH		32

using namespace X11;

/*!
	\brief Sets up internal variables needed by the X11Driver
*/
X11Driver::X11Driver(void) : BitmapDriver()
{
	// This link for sending mouse messages to the AppServer.
	// This is only to take the place of the Input Server for testing purposes.
	// Obviously, Cosmoe does not intend to use X11 except for testing.
	serverlink = new BPortLink(find_port(SERVER_INPUT_PORT));
	
	STRACE("X11Driver::X11Driver\n");

	drawsem = create_sem(1, "X11 draw semaphore");
}


X11Driver::~X11Driver()
{
	STRACE("X11Driver::~X11Driver\n");
	delete serverlink;
}

/*!
	\brief Translate X11 events into appserver events, as if they came
			right from the actual Input Server
*/
void XEventTranslator(void *arg)
{
	X11Driver *driver= (X11Driver*)arg;
	float x, y;
	XEvent x_event;
	status_t err;
	int quit = 0;

	STRACE("X11Driver::XEventTranslator\n");
	
	while( !quit )
	{
		XNextEvent(driver->display, &x_event);
		err = B_OK;
		switch(x_event.type)
		{
			case MotionNotify:
			{
				x=(float)((XMotionEvent *) &x_event)->x;
				y=(float)((XMotionEvent *) &x_event)->y;

				//STRACE("X11Driver::MouseMoved\n");
				uint32 buttons = 0;
				int64 time = (int64)real_time_clock();

				driver->serverlink->StartMessage(B_MOUSE_MOVED);
				driver->serverlink->Attach<int64>(time);
				driver->serverlink->Attach<float>(x);
				driver->serverlink->Attach<float>(y);
				driver->serverlink->Attach<int32>(buttons);
				err = driver->serverlink->Flush();
				break;
			}

			case Expose:{
				STRACE("Draw\n");
				//driver->GetTarget()->Bounds().PrintToStream();
				driver->Invalidate(driver->GetTarget()->Bounds());
				break;
			}

			case KeyPress:{
				STRACE("KeyDown\n");
#if 0
				char key = XLookupKeysym(&x_event.xkey, 0);
				BString string = key;
				int32 scancode, asciicode,repeatcount,modifiers;

				modifiers = 0;
				repeatcount = 1;
				scancode = ((XKeyEvent *)&x_event)->keycode;

				systime=(int64)real_time_clock();
				driver->serverlink->StartMessage(B_KEY_DOWN);
				driver->serverlink->Attach(&systime,sizeof(bigtime_t));
				driver->serverlink->Attach(scancode);
				//driver->serverlink->Attach(asciicode);
				driver->serverlink->Attach(repeatcount);
				driver->serverlink->Attach(modifiers);
				//driver->serverlink->Attach(utf8data,sizeof(int8)*3);
				driver->serverlink->Attach(string.Length()+1);
				driver->serverlink->Attach(string.String());
				//driver->serverlink->Attach(keyarray,sizeof(int8)*16);
				driver->serverlink->Flush();
#endif
				break;
			}

			case KeyRelease:{
				STRACE("KeyUp\n");
				break;
			}

			case ButtonPress:{
				STRACE("MouseDown\n");
				uint32 buttons = ((XButtonEvent *) &x_event)->button;
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
				err = driver->serverlink->Flush();
				break;
			}

			case ButtonRelease:{
				STRACE("MouseUp\n");
				uint32 mod = 0;
				int64 time = (int64)real_time_clock();

				driver->serverlink->StartMessage(B_MOUSE_UP);
				driver->serverlink->Attach<int64>(time);
				driver->serverlink->Attach<float>(x);
				driver->serverlink->Attach<float>(y);
				driver->serverlink->Attach<int32>(mod);
				err = driver->serverlink->Flush();
				break;
			}
			
			case DestroyNotify:
			case UnmapNotify:{
				quit = 1;
				break;
			}

			default:
				break;
		}

		if (err != B_OK)
			printf("Flush() went in the crapper with error %ld\n", err);

		XFlush(driver->display);
	}
	
	BPortLink applink(find_port(SERVER_PORT_NAME));

	STRACE("X11Driver: sending B_QUIT_REQUESTED message\n");
	applink.StartMessage(B_QUIT_REQUESTED);
	applink.Flush();

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
								B_RGBA32, 0);
	SetTarget(target);

	STRACE("passed SetTarget()\n");

	// Then we point XCreateImage at the UtilityBitmap's bits
	ximage = XCreateImage (display, CopyFromParent, depth, ZPixmap, 0,
						(char*)_target->Bits(), X11DRIVER_WIDTH, X11DRIVER_HEIGHT,
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

	BRect r(x1, y1, x2, y2);
	ValidateRect(&r);

	BitmapDriver::StrokePatternLine(x1, y1, x2, y2, d);

	Invalidate(r);
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


/*!
	\brief Refresh the X11 window with the contents of the ServerBitmap
	\param r      The BRect rectangle to refresh
*/
void X11Driver::Invalidate(const BRect &r)
{
	BRect damage = (r & _target->Bounds());

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
	int bytes_per_row = _target->BytesPerRow();
	RGBColor col(color);	// to avoid GetColor8/15/16() const issues

	switch(_target->BitsPerPixel())
	{
		case 8:
			{
				uint8 *fb = (uint8 *)_target->Bits() + y*bytes_per_row;
				uint8 color8 = col.GetColor8();
				fb[x] = color8;
			} break;
		case 15:
			{
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + y*bytes_per_row);
				uint16 color15 = col.GetColor15();
				fb[x] = color15;
			} break;
		case 16:
			{
				uint16 *fb = (uint16 *)((uint8 *)_target->Bits() + y*bytes_per_row);
				uint16 color16 = col.GetColor16();
				fb[x] = color16;
			} break;
		case 24:
		case 32:
			{
				uint32 *fb = (uint32 *)((uint8 *)_target->Bits() + y*bytes_per_row);
				rgb_color fill_color = color.GetColor32();
				uint32 color32 = (fill_color.alpha << 24) | (fill_color.red << 16) | (fill_color.green << 8) | (fill_color.blue);
				fb[x] = color32;
			} break;
		default:
			printf("Error: Unknown color space\n");
	}
}
