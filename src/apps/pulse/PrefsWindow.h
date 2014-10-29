//****************************************************************************************
//
//	File:		PrefsWindow.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//	Revised by: Steffen Yount
//
//****************************************************************************************

#ifndef PREFSWINDOW_H
#define PREFSWINDOW_H

#include <Window.h>
#include <Messenger.h>
#include "Prefs.h"

class PrefsWindow : public BWindow {
	public:
		PrefsWindow(BRect rect, const char *name, BMessenger *messenger, Prefs *prefs);
		void MessageReceived(BMessage *message);
		~PrefsWindow();
		void Quit();

		Prefs *prefs;
		BMessenger *messenger;
};

#endif

