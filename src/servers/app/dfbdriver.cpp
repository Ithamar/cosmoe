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
//	File Name:		dfbriver.cpp
//	Author:			Bill Hayden <hayden@haydentech.com>
//	Description:	Display driver which renders to DirectFB
//
//------------------------------------------------------------------------------


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <SupportDefs.h>
#include <macros.h>
#include <kernel.h>

#include <Bitmap.h>

#include "dfbdriver.h"


IDirectFB*	DFBObject = NULL;

#define DEBUG_DFB_DRIVER

#ifdef DEBUG_DFB_DRIVER
#include <stdio.h>
	#define STRACE(a) printf(a)
#else
	#define STRACE(a) /* nothing */
#endif

#define DFB_INPUT_DRIVER


/*!
	\brief Sets up internal variables needed by all DisplayDriver subclasses

	Subclasses should follow DisplayDriver's lead and use this function mostly
	for initializing data members.
*/
DirectFBDriver::DirectFBDriver() : DisplayDriver(), mDFBObject(NULL)
{
	STRACE("DFBDriver: In Constructor\n");
}

/*!
	\brief Deletes the locking semaphore

	Subclasses should use the destructor mostly for freeing allocated heap space.
*/
DirectFBDriver::~DirectFBDriver()
{
	STRACE("DFBDriver: In Destructor\n");
}


#ifdef DFB_INPUT_DRIVER
/*!
	\brief Translate DirectFB events into appserver events, as if they came
			right from the actual Input Server
*/
void DFBEventTranslator(void *arg)
{
	DFBInputEvent event;
	DirectFBDriver* driver = (DirectFBDriver*)arg;
	bool loop = true;
	float x = 0.0f;
	float y = 0.0f;

	STRACE("Entering DFBEventTranslator\n");

	while(loop)
	{
		snooze( 10000 );

		driver->mEventBuffer->WaitForEvent(driver->mEventBuffer);

		while (driver->mEventBuffer->GetEvent(driver->mEventBuffer, DFB_EVENT(&event)) == DFB_OK)
		{
			switch (event.type)
			{
				case DIET_AXISMOTION:
				{
					if (event.axis == DIAI_X)
						x=(float)event.axisabs;
					else
						y=(float)event.axisabs;

					break;
				}

				case DIET_KEYPRESS:{
					STRACE("DIET_KEYPRESS\n");
					switch (DFB_LOWER_CASE (event.key_symbol))
					{
						case DIKS_ESCAPE:
						case DIKS_EXIT:
						case 'q':
							loop = false;
							break;
					}
					break;
				}

				case DIET_KEYRELEASE:{
					STRACE("DIET_KEYRELEASE\n");
					break;
				}
	
				case DIET_BUTTONPRESS:{
					STRACE("MouseDown\n");
					break;
				}
	
				case DIET_BUTTONRELEASE:{
					STRACE("MouseUp\n");
					break;
				}

				default:
					break;
			}
		}
	}

	STRACE("Leaving DFBEventTranslator\n");
}
#endif /* DFB_INPUT_DRIVER */

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not

	Initialize sets up the driver for display, including the initial clearing
	of the screen. If things do not go as they should, false should be returned.
*/
bool DirectFBDriver::Initialize(void)
{
	DFBResult				err;
	DFBSurfaceDescription	dsc;
	DFBDisplayLayerConfig	layerConfig;

	STRACE("DFBDriver: in Initialize()\n");
	
	if (DirectFBInit(NULL, NULL) != DFB_OK)
		return false;

	STRACE("DirectFBInit succeeded\n");

	if (DirectFBCreate( &mDFBObject ) != DFB_OK)
		return false;

	STRACE("DirectFBCreate succeeded\n");

	mDFBObject->SetCooperativeLevel( mDFBObject, DFSCL_FULLSCREEN );

	STRACE("DFBDriver: About to set display to 32bpp\n");

	// Grab our current width and height
	mDFBObject->GetDisplayLayer( mDFBObject, DLID_PRIMARY, &mLayer );
	mLayer->GetConfiguration( mLayer, &layerConfig );

	fDisplayMode.virtual_width=layerConfig.width;
	fDisplayMode.virtual_height=layerConfig.height;
	fDisplayMode.space=B_RGBA32;

	// See if we can create our primary surface

	dsc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_CAPS | DSDESC_WIDTH | DSDESC_HEIGHT);
	dsc.caps  = (DFBSurfaceCapabilities)(DSCAPS_PRIMARY | DSCAPS_FLIPPING);
	dsc.width = layerConfig.width;
	dsc.height = layerConfig.height;

#ifdef DEBUG_DFB_DRIVER
	printf("DFBDriver: About to create a %dx%d primary surface\n", dsc.width, dsc.height);
#endif

	err = mDFBObject->CreateSurface( mDFBObject, &dsc, &mPrimarySurface );
	if (err != DFB_OK)
	{
		STRACE("DFBDriver: Failed creating primary surface\n");
		mLayer->Release( mLayer );
		mDFBObject->Release( mDFBObject );
		return false;
	}

#ifdef DFB_INPUT_DRIVER

	STRACE("DFBDriver: Initialize input\n");

	mDFBObject->CreateInputEventBuffer( mDFBObject, DICAPS_ALL,
													DFB_FALSE,
													&mEventBuffer );

	// Create a new thread for mouse and key events
	pthread_t input_thread;
	pthread_create (&input_thread,
					NULL,
					(void *(*) (void *))&DFBEventTranslator,
					(void *) this);
#endif /* DFB_INPUT_DRIVER */

	return true;
}

/*!
	\brief Shuts down the driver's video subsystem

	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void DirectFBDriver::Shutdown(void)
{
	STRACE( "DFBDriver: in Shutdown()\n" );
	mLayer->Release( mLayer );
	mPrimarySurface->Release( mPrimarySurface );
	mDFBObject->Release( mDFBObject );
}


/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param color The color used when filling the rectangle
*/
void DirectFBDriver::FillSolidRect(const BRect& r, const RGBColor& color)
{
	rgb_color col = color.GetColor32();
	
	mPrimarySurface->SetColor( mPrimarySurface, col.red,
												col.green,
												col.blue,
												col.alpha);

	mPrimarySurface->FillRectangle(mPrimarySurface, (long)r.left,
													(long)r.top,
													r.IntegerWidth(),
													r.IntegerHeight());

	DFBRegion	aFlipRegion;

	aFlipRegion.x1 = (long)r.left;
	aFlipRegion.x2 = (long)r.right;
	aFlipRegion.y1 = (long)r.top;
	aFlipRegion.y2 = (long)r.bottom;

	mPrimarySurface->Flip( mPrimarySurface, &aFlipRegion, (DFBSurfaceFlipFlags)0 );
}

/*!
	\brief Called for all BView::FillRect calls
	\param r BRect to be filled. Guaranteed to be in the frame buffer's coordinate space
	\param color The color used when filling the rectangle
*/
void DirectFBDriver::StrokeSolidRect(const BRect& r, const RGBColor& color)
{
	rgb_color col = color.GetColor32();
	
	mPrimarySurface->SetColor( mPrimarySurface, col.red,
												col.green,
												col.blue,
												col.alpha);

	mPrimarySurface->DrawRectangle(mPrimarySurface, (long)r.left,
													(long)r.top,
													r.IntegerWidth(),
													r.IntegerHeight());

	DFBRegion	aFlipRegion;

	aFlipRegion.x1 = (long)r.left;
	aFlipRegion.x2 = (long)r.right;
	aFlipRegion.y1 = (long)r.top;
	aFlipRegion.y2 = (long)r.bottom;

	mPrimarySurface->Flip( mPrimarySurface, &aFlipRegion, (DFBSurfaceFlipFlags)0 );
}


void DirectFBDriver::StrokeSolidLine(int32 x1, int32 y1, int32 x2, int32 y2, const RGBColor &color)
{
	DFBRegion	clippingRect;
	rgb_color	col = color.GetColor32();

	clippingRect.x1 = min_c(x1, x2);
	clippingRect.y1 = min_c(y1, y2);
	clippingRect.x2 = max_c(x1, x2);
	clippingRect.y2 = max_c(y1, y2);

	mPrimarySurface->SetColor( mPrimarySurface, col.red,
												col.green,
												col.blue,
												col.alpha);
	mPrimarySurface->DrawLine(mPrimarySurface, x1, y1, x2, y2);

	mPrimarySurface->Flip(mPrimarySurface, &clippingRect, (DFBSurfaceFlipFlags)0);
}


bool DirectFBDriver::AcquireBuffer(FBBitmap *bmp)
{
	void* data;
	int pitch;

	if (bmp && mPrimarySurface->Lock(mPrimarySurface,
									  (DFBSurfaceLockFlags)(DSLF_READ | DSLF_WRITE),
									  &data,
									  &pitch) == 0)
	{
		bmp->SetBytesPerRow(pitch);
		bmp->SetSpace(B_RGBA32);
		bmp->SetSize(fDisplayMode.virtual_width - 1,
					 fDisplayMode.virtual_height - 1);
		bmp->SetBuffer(data);
		bmp->SetBitsPerPixel(B_RGBA32, pitch);
		return true;
	}

	STRACE( "DFBDriver: AcquireBuffer() failed\n" );
	return false;
}


void DirectFBDriver::ReleaseBuffer()
{
	mPrimarySurface->Unlock(mPrimarySurface);
}

/*!
	\brief Refresh the DirectFB bitmap
	\param r      The BRect rectangle to refresh
*/
void DirectFBDriver::Invalidate(const BRect &r)
{
	DFBRegion	updateRect;

	updateRect.x1 = (long)r.left;
	updateRect.y1 = (long)r.top;
	updateRect.x2 = (long)r.right;
	updateRect.y2 = (long)r.bottom;
	
	mPrimarySurface->Flip(mPrimarySurface, &updateRect, (DFBSurfaceFlipFlags)0);
}

