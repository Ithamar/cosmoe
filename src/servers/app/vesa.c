//------------------------------------------------------------------------------
//	Copyright (c) 2003, Tom Marshall
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
//	File Name:		vesa.cpp
//	Authors:		Tom Marshall (tommy@tig-grr.com)
//------------------------------------------------------------------------------


#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include <Debug.h>
#include <SupportDefs.h>
#include <vesa_gfx.h>

int fb_fd = -1;
struct fb_fix_screeninfo fb_fixsi;
struct fb_var_screeninfo fb_varsi;
size_t fb_len;
void* fb_ptr = NULL;


int get_vesa_mode_info( VESA_Mode_Info_s* psVesaModeInfo, uint32 nModeNr )
{
	memset( psVesaModeInfo, 0, sizeof(VESA_Mode_Info_s) );
	if( nModeNr != 1 || !fb_ptr )
	{
		dbprintf( "get_vesa_mode_info(): unknown mode %u\n", nModeNr );
		return EINVAL;
	}

	psVesaModeInfo->ModeAttributes = 0; /* XXX */
	psVesaModeInfo->WinAAttributes = 0; /* XXX */
	psVesaModeInfo->WinBAttributes = 0; /* XXX */
	psVesaModeInfo->WinGranularity = 0; /* XXX */
	psVesaModeInfo->WinSize = 0;        /* XXX */
	psVesaModeInfo->WinASegment = 0;    /* XXX */
	psVesaModeInfo->WinBSegment = 0;    /* XXX */
	psVesaModeInfo->WinFuncPtr = 0;     /* XXX */

	psVesaModeInfo->BytesPerScanLine = fb_fixsi.line_length;
	psVesaModeInfo->XResolution = fb_varsi.xres;
	psVesaModeInfo->YResolution = fb_varsi.yres;
	psVesaModeInfo->XCharSize = 0; /* XXX */
	psVesaModeInfo->YCharSize = 0; /* XXX */
	psVesaModeInfo->NumberOfPlanes = 1;
	psVesaModeInfo->BitsPerPixel = fb_varsi.bits_per_pixel;
	psVesaModeInfo->NumberOfBanks = 1;
	psVesaModeInfo->MemoryModel = 0;
	psVesaModeInfo->BankSize = 0;
	psVesaModeInfo->NumberOfImagePages = 0;
	psVesaModeInfo->Reserved = 0;

	/* TODO: figure out other depth mappings .. probably need a table */
	switch( fb_varsi.bits_per_pixel )
	{
	case 16:
		psVesaModeInfo->RedMaskSize = 5; psVesaModeInfo->RedFieldPosition = 11;
		psVesaModeInfo->GreenMaskSize = 6; psVesaModeInfo->GreenFieldPosition = 5;
		psVesaModeInfo->BlueMaskSize = 5; psVesaModeInfo->BlueFieldPosition = 0;
		psVesaModeInfo->RsvdMaskSize = 0; psVesaModeInfo->RsvdFieldPosition = 0;
		psVesaModeInfo->DirectColorModeInfo = 0; /* XXX */
		break;
	case 32:
		psVesaModeInfo->RedMaskSize = 8; psVesaModeInfo->RedFieldPosition = 16;
		psVesaModeInfo->GreenMaskSize = 8; psVesaModeInfo->GreenFieldPosition = 8;
		psVesaModeInfo->BlueMaskSize = 8; psVesaModeInfo->BlueFieldPosition = 0;
		psVesaModeInfo->RsvdMaskSize = 8; psVesaModeInfo->RsvdFieldPosition = 24;
		psVesaModeInfo->DirectColorModeInfo = 0; /* XXX */
		break;
	default:
		dbprintf( "get_vesa_mode_info(): unknown color depth %i\n", fb_varsi.bits_per_pixel );
		return EINVAL;
	}
	psVesaModeInfo->DirectColorModeInfo = 0;
	psVesaModeInfo->PhysBasePtr = (int32)fb_ptr;
	psVesaModeInfo->OffScreenMemOffset = 0;
	psVesaModeInfo->OffScreenMemSize = 0;

	return 0;
}


int get_vesa_info( Vesa_Info_s* psVesaInfo, uint16* pnModeList, int nMaxModeCount )
{
	int n = 0;

	if( nMaxModeCount < 1 )
	{
		return 0;
	}

	memset( psVesaInfo, 0, sizeof(Vesa_Info_s) );
	for( n = 0; n < nMaxModeCount; n++ )
	{
		pnModeList[n] = 0;
	}

	/* open framebuffer device */
	if( fb_fd == -1 )
	{
		const char* fbdev = getenv("FRAMEBUFFER");
		if( !fbdev ) fbdev = "/dev/fb0";
		fb_fd = open( fbdev, O_RDWR );
		if( -1 == fb_fd )
		{
			dbprintf( "get_vesa_info(): open( %s ) failed: %s\n", fbdev, strerror(errno) );
			return 0;
		}
	}

	if( fb_ptr == NULL )
	{
		/* get relevant info */
		if( 0 == ioctl( fb_fd, FBIOGET_FSCREENINFO, &fb_fixsi ) &&
			0 == ioctl( fb_fd, FBIOGET_VSCREENINFO, &fb_varsi ) )
		{
			fb_len = fb_fixsi.line_length * fb_varsi.yres_virtual;

			/* okay now map the buffer */
			fb_ptr = mmap( NULL, fb_len, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fb_fd, 0 );
			if( MAP_FAILED == fb_ptr )
			{
				dbprintf( "get_vesa_info(): mmap() failed: %s\n", strerror(errno) );
				fb_ptr = NULL;
				return 0;
			}
			/* we win */
			pnModeList[0] = 1;
			/* the psVesaInfo struct appears totally unused on return */
			memcpy( psVesaInfo->VesaSignature, "VBE2", 4 );
			psVesaInfo->VesaVersion = 2;
		}
	}

	return 1;
}


int set_vesa_mode( uint32 nMode )
{
	dbprintf( "set_vesa_mode( %u )\n", nMode );

	/* we only support one mode: 0 */
	return ( nMode == 0 );
}
