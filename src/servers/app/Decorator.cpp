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
//	File Name:		Decorator.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Base class for window decorators
//  
//------------------------------------------------------------------------------
#include <Region.h>
#include "ColorSet.h"
#include "Decorator.h"
#include <stdio.h>

/*!
	\brief Constructor
	\param rect Size of client area
	\param wlook style of window look. See Window.h
	\param wfeel style of window feel. See Window.h
	\param wflags various window flags. See Window.h
	
	Does general initialization of internal data members and creates a colorset
	object. 
*/
Decorator::Decorator(Layer* pcLayer, int32 wlook, int32 wfeel, uint32 wflags)
{
	_close_state=false;
	_minimize_state=false;
	_zoom_state=false;
	_has_focus=false;
	_title_string=new BString;

	_look=wlook;
	_feel=wfeel;
	_flags=wflags;

	_colors=new ColorSet();
	_colors->SetToDefaults();

	m_pcLayer = pcLayer;
}


/*!
	\brief Destructor
	
	Frees the color set and the title string
*/
Decorator::~Decorator(void)
{
	if(_colors!=NULL)
	{
		delete _colors;
		_colors=NULL;
	}
	if(_title_string)
		delete _title_string;
}

/*!
	\brief Updates the decorator's color set
	\param cset The color set to update from
*/
void Decorator::SetColors(const ColorSet &cset)
{
	_colors->SetColors(cset);
	_SetColors();
}

/*!
	\brief Sets the close button's value.
	\param is_down Whether the button is down or not
	
	Note that this does not update the button's look - it just updates the
	internal button value
*/
void Decorator::SetClose(bool is_down)
{
	_close_state=is_down;
}

/*!
	\brief Sets the minimize button's value.
	\param is_down Whether the button is down or not
	
	Note that this does not update the button's look - it just updates the
	internal button value
*/
void Decorator::SetMinimize(bool is_down)
{
	_zoom_state=is_down;
}

/*!
	\brief Sets the zoom button's value.
	\param is_down Whether the button is down or not
	
	Note that this does not update the button's look - it just updates the
	internal button value
*/
void Decorator::SetZoom(bool is_down)
{
	_minimize_state=is_down;
}

/*!
	\brief Sets the decorator's window flags
	\param wflags New value for the flags
	
	While this call will not update the screen, it will affect how future
	updates work and immediately affects input handling.
*/
void Decorator::SetFlags(uint32 wflags)
{
	_flags=wflags;
}

/*!
	\brief Sets the decorator's window feel
	\param wflags New value for the feel
	
	While this call will not update the screen, it will affect how future
	updates work and immediately affects input handling.
*/
void Decorator::SetFeel(int32 wfeel)
{
	_feel=wfeel;
}

/*!
	\brief Sets the decorator's window look
	\param wflags New value for the look
	
	While this call will not update the screen, it will affect how future
	updates work and immediately affects input handling.
*/
void Decorator::SetLook(int32 wlook)
{
	_look=wlook;
}

/*!
	\brief Returns the value of the close button
	\return true if down, false if up
*/
bool Decorator::GetClose(void)
{
	return _close_state;
}

/*!
	\brief Returns the value of the minimize button
	\return true if down, false if up
*/
bool Decorator::GetMinimize(void)
{
	return _minimize_state;
}

/*!
	\brief Returns the value of the zoom button
	\return true if down, false if up
*/
bool Decorator::GetZoom(void)
{
	return _zoom_state;
}

/*!
	\brief Returns the decorator's window look
	\return the decorator's window look
*/
int32 Decorator::GetLook(void)
{
	return _look;
}

/*!
	\brief Returns the decorator's window feel
	\return the decorator's window feel
*/
int32 Decorator::GetFeel(void)
{
	return _feel;
}

/*!
	\brief Returns the decorator's window flags
	\return the decorator's window flags
*/
uint32 Decorator::GetFlags(void)
{
	return _flags;
}

/*!
	\brief Updates the value of the decorator title
	\param string New title value
*/
void Decorator::SetTitle(const char *string)
{
	_title_string->SetTo(string);
}

/*!
	\brief Returns the decorator's title
	\return the decorator's title
*/
const char *Decorator::GetTitle(void)
{
	return _title_string->String();
}

/*!
	\brief Returns the decorator's border rectangle
	\return the decorator's border rectangle
*/

BRect Decorator::GetBorderRect(void){
	return _borderrect;
}

/*!
	\brief Returns the decorator's tab rectangle
	\return the decorator's tab rectangle
*/

BRect Decorator::GetTabRect(void){
	return _tabrect;
}

/*!
	\brief Changes the focus value of the decorator
	\param is_active True if active, false if not
	
	While this call will not update the screen, it will affect how future
	updates work.
*/
void Decorator::SetFocus(bool is_active)
{
	_has_focus=is_active;
	_SetFocus();
}

/*!
	\brief Updates the decorator's look in the area r
	\param r The area to update.
	
	The default version updates all areas which intersect the frame and tab.
*/
void Decorator::Draw(BRect r)
{
}

/*!
	\brief Hook function for when the color set is updated
	
	This function is called after the decorator's color set is updated. Quite useful 
	if the decorator uses colors based on those in the system.
*/
void Decorator::_SetColors(void)
{
}

/*!
	\brief Returns the "footprint" of the entire window, including decorator
	\param region Region to be changed to represent the window's screen footprint
	
	This function is required by all subclasses.
*/
void Decorator::GetFootprint(BRegion *region)
{
}

Layer* Decorator::GetLayer() const
{
    return( m_pcLayer );
}

/*!
	\brief Performs hit-testing for the decorator
	\return The type of area clicked

	Clicked is called whenever it has been determined that the window has received a 
	mouse click. The default version returns DEC_NONE. A subclass may use any or all
	of them.
	
	Click type : Action taken by the server
	
	- \c DEC_NONE : Do nothing
	- \c DEC_ZOOM : Handles the zoom button (setting states, etc)
	- \c DEC_CLOSE : Handles the close button (setting states, etc)
	- \c DEC_MINIMIZE : Handles the minimize button (setting states, etc)
	- \c DEC_TAB : Currently unused
	- \c DEC_DRAG : Moves the window to the front and prepares to move the window
	- \c DEC_MOVETOBACK : Moves the window to the back of the stack
	- \c DEC_MOVETOFRONT : Moves the window to the front of the stack
	- \c DEC_SLIDETAB : Initiates tab-sliding, including calling SlideTab()

	- \c DEC_RESIZE : Handle window resizing as appropriate
	- \c DEC_RESIZE_L
	- \c DEC_RESIZE_T
	- \c DEC_RESIZE_R
	- \c DEC_RESIZE_B
	- \c DEC_RESIZE_LT
	- \c DEC_RESIZE_RT
	- \c DEC_RESIZE_LB
	- \c DEC_RESIZE_RB

	This function is required by all subclasses.
	
*/
click_type Decorator::Clicked(BPoint pt, int32 buttons, int32 modifiers)
{
	return DEC_NONE;
}

//! Hook function called when the decorator changes focus
void Decorator::_SetFocus(void)
{
}

//! Function for calculating layout for the decorator
void Decorator::_DoLayout(void)
{

}
