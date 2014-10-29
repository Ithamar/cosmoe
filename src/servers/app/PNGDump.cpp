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
//	File Name:		PNGDump.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Function for saving a generic framebuffer to a PNG file
//  
//------------------------------------------------------------------------------
#include <Rect.h>
#include <Bitmap.h>
#include <stdio.h>
#include <unistd.h>
#include <png.h>
#include <stdlib.h>

#include "DisplayDriver.h"
#include <ServerBitmap.h>
#include "Layer.h"
#include "AppServer.h"

#define DEBUG_PNGDUMP

void SaveToPNG(const char *filename, const BRect &bounds, color_space space, 
	const void *bits, const int32 &bitslength, const int32 bytesperrow)
{
#ifdef DEBUG_PNGDUMP
printf("SaveToPNG: %s ",filename);
bounds.PrintToStream();
#endif
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	
	fp=fopen(filename, "wb");
	if(fp==NULL)
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't open file\n");
#endif
		return;
	}

      /* Create and initialize the png_struct with the desired error handler
       * functions.  If you want to use the default stderr and longjump method,
       * you can supply NULL for the last three parameters.  We also check that
       * the library version is compatible with the one used at compile time,
       * in case we are using dynamically linked libraries.  REQUIRED.
       */
	png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
//      png_voidp user_error_ptr, user_error_fn, user_warning_fn);

	if(png_ptr==NULL)
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't create write struct\n");
#endif
		fclose(fp);
		return;
	}

      /* Allocate/initialize the image information data.  REQUIRED */
	info_ptr=png_create_info_struct(png_ptr);
	if(info_ptr==NULL)
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't create info struct\n");
#endif
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		return;
	}

      /* Set error handling.  REQUIRED if you aren't supplying your own
       * error handling functions in the png_create_write_struct() call.
       */
	if(setjmp(png_ptr->jmpbuf))
	{
#ifdef DEBUG_PNGDUMP
printf("Couldn't set jump\n");
#endif
          /* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		return;
	}

	png_init_io(png_ptr, fp);

	png_set_compression_level(png_ptr,Z_NO_COMPRESSION);

	png_set_bgr(png_ptr);

      /* Set the image information here.  Width and height are up to 2^31,
       * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
       * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
       * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
       * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
       * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
       * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
       */
    printf( "Init info header\n" );
   	png_set_IHDR(png_ptr, info_ptr, bounds.IntegerWidth(), bounds.IntegerHeight(), 8, PNG_COLOR_TYPE_RGB_ALPHA,
	PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
   
      /* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
      /* note that if sRGB is present the cHRM chunk must be ignored
       * on read and must be written in accordance with the sRGB profile */

      /* Write the file header information.  REQUIRED */
    printf( "Write info header\n" );
	png_write_info(png_ptr, info_ptr);
	
	png_byte *row_pointers[bounds.IntegerHeight()];
	png_byte *index=(png_byte*)bits;
	for(int32 i=0;i<bounds.IntegerHeight();i++)
	{
		row_pointers[i]=index;
		index+=bytesperrow;
	}
	png_write_image(png_ptr, row_pointers);

      /* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end(png_ptr, info_ptr);

      /* if you malloced the palette, free it here */
    free(info_ptr->palette);

      /* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}

void ScreenShot()
{
#if 1
	g_cLayerGate.Close();
	if ( g_pcTopView != NULL && g_pcTopView->GetBitmap() != NULL )
	{
		ServerBitmap* bm = g_pcTopView->GetBitmap();
		SaveToPNG( "/tmp/screenshot.png", bm->Bounds(),
			bm->ColorSpace(), bm->Bits(), bm->BitsLength(), bm->BytesPerRow() );
		printf( "Done\n" );
	}
	g_cLayerGate.Open();
#endif
}

