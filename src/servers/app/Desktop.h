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
//	File Name:		Desktop.h
//	Author:			Adi Oanca <adioanca@mymail.ro>
//	Description:	Class used to encapsulate desktop management
//
//------------------------------------------------------------------------------
#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include <Locker.h>
#include <List.h>
#include <Menu.h>
#include <InterfaceDefs.h>
#include <DisplayDriver.h> // for display_mode

class ServerWindow;
class Screen;
class Layer;
class BMessage;
class WinBorder;
class BPortLink;

class Desktop
{
public:
	// startup methods
	Desktop(void);
	~Desktop(void);
	void Init(void);

	// 1-BigScreen or n-SmallScreens
	void InitMode(void);

	// Methods for multiple monitors.
	Screen *ScreenAt(int32 index) const;
	int32 ScreenCount(void) const;
	Screen *ActiveScreen(void) const;
	
	DisplayDriver *GetDisplayDriver(void) const;
	
	// Methods for layer(WinBorder) manipulation.

	// Input related methods
	void MouseEventHandler(int32 code, BPortLink& link);
	void KeyboardEventHandler(int32 code, BPortLink& link);
	
	void SetDragMessage(BMessage *msg);
	BMessage *DragMessage(void) const;
	
	// Methods for various desktop stuff handled by the server
	void SetScrollBarInfo(const scroll_bar_info &info);
	scroll_bar_info		ScrollBarInfo(void) const;
	
	void SetMenuInfo(const menu_info &info);
	menu_info MenuInfo(void) const;
	
	
	void UseFFMouse(const bool &useffm);
	bool FFMouseInUse(void) const;
	void SetFFMouseMode(const mode_mouse &value);
	mode_mouse FFMouseMode(void) const;
	
	// Debugging methods
	void PrintToStream(void);

	BLocker fGeneralLock;
	BLocker fLayerLock;
	BList fWinBorderList;

private:
	BMessage *fDragMessage;
	
	BList fRootLayerList;

	BList fScreenList;
	Screen *fActiveScreen;
	
	scroll_bar_info fScrollBarInfo;
	menu_info fMenuInfo;
	mode_mouse fMouseMode;
	bool fFFMouseMode;
	int32 fScreenShotIndex;
};

int  get_desktop_config( int32* pnActiveDesktop, display_mode* psMode, std::string* pcBackdropPath );

int get_active_desktop();

void AddWindowToDesktop(ServerWindow *win);
void RemoveWindowFromDesktop(ServerWindow *win);
ServerWindow *GetActiveWindow(bool bIgnoreSystemWindows);
void SetActiveWindow(ServerWindow *win, bool bNotifyPrevious = true);

extern Desktop *desktop;

#endif // _DESKTOP_H_
