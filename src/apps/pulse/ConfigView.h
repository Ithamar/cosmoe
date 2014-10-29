//****************************************************************************************
//
//	File:		ConfigView.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//	Revised by: Steffen Yount
//
//****************************************************************************************

#ifndef CONFIGVIEW_H
#define CONFIGVIEW_H

#include <CheckBox.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <ColorControl.h>
#include "Prefs.h"





class RTColorControl : public BColorControl {
	public:
		RTColorControl(BPoint point, BMessage *message);
		void SetValue(int32 color);
};

class ConfigView : public BView {
	public:
		ConfigView(BRect rect, const char *name, int mode, Prefs *prefs);
		void AttachedToWindow();
		void MessageReceived(BMessage *message);
		void UpdateDeskbarIconWidth();
		
	private:
		void ResetDefaults();
		bool first_time_attached;
		int mode;
		
		RTColorControl *colorcontrol;
		// For Normal
		BCheckBox *fadecolors;
		// For Mini and Deskbar
		BRadioButton *active, *idle, *frame;
		// For Deskbar
		BTextControl *iconwidth;
};

#endif
