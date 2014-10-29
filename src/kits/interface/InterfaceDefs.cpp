//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
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
//	File Name:		InterfaceDefs.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Caz <turok2@currantbun.com>
//					Axel Dörfler <axeld@pinc-software.de>
//	Description:	Global functions and variables for the Interface Kit
//
//------------------------------------------------------------------------------

#include <AppServerLink.h>
#include <InterfaceDefs.h>
#include <ServerProtocol.h>
#include <ScrollBar.h>
#include <Screen.h>
#include <Roster.h>
#include <Menu.h>
#include <stdlib.h>
#include <TextView.h>

static rgb_color ui_colors[] =
{
	{ 0xd8, 0xd8, 0xd8, 0xff },	// B_PANEL_BACKGROUND_COLOR
	{ 0xcc, 0xcc, 0xcc, 0xff },	// B_MENU_BACKGROUND_COLOR
	{ 0xd8, 0xd8, 0xd8, 0xff },	// B_WINDOW_TAB_COLOR
	{ 0x66, 0x88, 0xbb, 0xff },	// B_KEYBOARD_NAVIGATION_COLOR
	{ 0x66, 0x66, 0x66, 0xff },	// B_DESKTOP_COLOR
	{ 0x66, 0x88, 0xbb, 0xff },	// B_MENU_SELECTION_BACKGROUND_COLOR
	{ 0x00, 0x00, 0x00, 0xff },	// B_MENU_ITEM_TEXT_COLOR
	{ 0x00, 0x00, 0x00, 0xff }	// B_MENU_SELECTED_ITEM_TEXT_COLOR
};

/*
	rgb_color( 0xaa, 0xaa, 0xaa, 0xff ),        // COL_NORMAL
	rgb_color( 0xff, 0xff, 0xff, 0xff ),        // COL_SHINE
	rgb_color( 0x00, 0x00, 0x00, 0xff ),        // COL_SHADOW
	rgb_color( 0x66, 0x88, 0xbb, 0xff ),        // COL_SEL_WND_BORDER
	rgb_color( 0x78, 0x78, 0x78, 0xff ),        // COL_NORMAL_WND_BORDER
	rgb_color( 0x00, 0x00, 0x00, 0xff ),        // COL_MENU_TEXT
	rgb_color( 0x00, 0x00, 0x00, 0xff ),        // COL_SEL_MENU_TEXT
	rgb_color( 0xcc, 0xcc, 0xcc, 0xff ),        // COL_MENU_BACKGROUND
	rgb_color( 0x66, 0x88, 0xbb, 0xff ),        // COL_SEL_MENU_BACKGROUND
	rgb_color( 0x78, 0x78, 0x78, 0xff ),        // COL_SCROLLBAR_BG
	rgb_color( 0xaa, 0xaa, 0xaa, 0xff ),        // COL_SCROLLBAR_KNOB
	rgb_color( 0x78, 0x78, 0x78, 0xff ),        // COL_LISTVIEW_TAB
	rgb_color( 0xff, 0xff, 0xff, 0xff )         // COL_LISTVIEW_TAB_TEXT
*/

// Private definitions not placed in public headers
extern "C" void _init_global_fonts();
extern "C" status_t _fini_interface_kit_();
extern status_t _control_input_server_(BMessage *command, BMessage *reply);

using namespace BPrivate;


// InterfaceDefs.h



_IMPEXP_BE status_t
set_click_speed(bigtime_t speed)
{
	return B_OK;
}

_IMPEXP_BE status_t
set_mouse_acceleration(int32 speed)
{
	return B_OK;
}

status_t		get_scroll_bar_info(scroll_bar_info *info)
{
	info->proportional = true;
	info->double_arrows = true;
	info->knob = 1;
	info->min_knob_size = 14;

	return B_OK;
}


_IMPEXP_BE uint32
modifiers()
{
	return 0UL;
}


status_t		set_scroll_bar_info(scroll_bar_info *info)
{
	return B_OK;
}

_IMPEXP_BE status_t
_restore_key_map_()
{
	return B_OK;	
}


_IMPEXP_BE rgb_color
keyboard_navigation_color()
{
	// Queries the app_server
	return ui_color(B_KEYBOARD_NAVIGATION_COLOR);
}


_IMPEXP_BE void
run_select_printer_panel()
{
	// Launches the Printer prefs app via the Roster
}


_IMPEXP_BE void
run_add_printer_panel()
{
	// Launches the Printer prefs app via the Roster and asks it to 
	// add a printer
	// TODO: Implement
}


_IMPEXP_BE void
run_be_about()
{
	// Unsure about how to implement this.
	// TODO: Implement
}


_IMPEXP_BE void
set_focus_follows_mouse(bool follow)
{
}

_IMPEXP_BE bool
focus_follows_mouse()
{
	return false;
}

rgb_color	ui_color(color_which which)
{
	// FIXME: no bounds checking

	return ui_colors[which-1];
}

_IMPEXP_BE rgb_color
tint_color(rgb_color color, float tint)
{
	rgb_color result;

	#define LIGHTEN(x) ((uint8)(255.0f - (255.0f - x) * tint))
	#define DARKEN(x)  ((uint8)(x * (2 - tint)))

	if (tint < 1.0f)
	{
		result.red   = LIGHTEN(color.red);
		result.green = LIGHTEN(color.green);
		result.blue  = LIGHTEN(color.blue);
		result.alpha = color.alpha;
	}
	else
	{
		result.red   = DARKEN(color.red);
		result.green = DARKEN(color.green);
		result.blue  = DARKEN(color.blue);
		result.alpha = color.alpha;
	}

	#undef LIGHTEN
	#undef DARKEN

	return result;
}


extern "C" status_t
_init_interface_kit_()
{
	status_t result = get_menu_info(&BMenu::sMenuInfo);
	if (result != B_OK)  
		return result;

	//TODO: fill the other static members
		
	return B_OK;
}


extern "C" status_t
_fini_interface_kit_()
{
	//TODO: Implement ?
	
	return B_OK;
}


extern "C" void
_init_global_fonts()
{
	// This function will initialize the client-side font list
	// TODO: Implement
}

