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
//	File Name:		PopUpMenu.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//	Description:	BPopUpMenu represents a menu that pops up when you
//                  activate it.
//------------------------------------------------------------------------------

 
#include <PopUpMenu.h>


struct popup_menu_data 
{
	BPopUpMenu *object;
	BWindow *window;
	BMenuItem *selected;
	
	BPoint where;
	BRect rect;
	
	bool async;
	bool auto_invoke;
	bool start_opened;
	bool use_rect;
	
	sem_id lock;
};


BPopUpMenu::BPopUpMenu(	const char *inName,
						bool inRadioMode,
						bool inAutoRename,
						menu_layout inLayout ) : BMenu(inName, inLayout)
{
	mAutoRename = inAutoRename;
	
	if (inRadioMode || inAutoRename)
		SetRadioMode(true);
}


BPopUpMenu::~BPopUpMenu()
{
}


BMenuItem*	BPopUpMenu::Go( BPoint where,
							bool delivers_message,
							bool open_anyway,
							bool async)
{
	return NULL;
}


BMenuItem*	BPopUpMenu::Go( BPoint where,
							bool delivers_message,
							bool open_anyway,
							BRect click_to_open,
							bool async )
{
	return NULL;
}

void	BPopUpMenu::MessageReceived(BMessage *pcMessage)
{
}


void	BPopUpMenu::MouseDown(BPoint pt)
{
}


void	BPopUpMenu::MouseUp(BPoint pt)
{
}


void	BPopUpMenu::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
}


void	BPopUpMenu::AttachedToWindow()
{
}


void	BPopUpMenu::DetachedFromWindow()
{
}


void	BPopUpMenu::FrameMoved(BPoint new_position)
{
}


void	BPopUpMenu::FrameResized(float inWidth, float inHeight)
{
}


void	BPopUpMenu::ResizeToPreferred()
{
}


void	BPopUpMenu::GetPreferredSize(float *outWidth, float *outHeight)
{
}


void	BPopUpMenu::MakeFocus(bool state)
{
}


void	BPopUpMenu::AllAttached()
{
}


void	BPopUpMenu::AllDetached()
{
}


BPoint	BPopUpMenu::ScreenLocation()
{
	if (!IsRadioMode())
		return BMenu::ScreenLocation();
	
	// FIXME: figure out where the menu should be popped up
	
	return BMenu::ScreenLocation();
}


void	BPopUpMenu::_ReservedPopUpMenu1()	{}
void	BPopUpMenu::_ReservedPopUpMenu2()	{}
void	BPopUpMenu::_ReservedPopUpMenu3()	{}

