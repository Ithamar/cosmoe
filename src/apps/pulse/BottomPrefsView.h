//****************************************************************************************
//
//	File:		BottomPrefsView.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//	Revised by: Steffen Yount
//
//****************************************************************************************

#ifndef BOTTOMPREFSVIEW_H
#define BOTTOMPREFSVIEW_H

#include <View.h>
#include "Prefs.h"





class BottomPrefsView : public BView {
	public:
		BottomPrefsView(BRect rect, const char *name);
		void Draw(BRect rect);
};

#endif

