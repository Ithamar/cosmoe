//****************************************************************************************
//
//	File:		PulseApp.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//	Revised by: Steffen Yount
//
//****************************************************************************************

#ifndef PULSEAPP_H
#define PULSEAPP_H

#include <Application.h>
#include "Prefs.h"

bool LastEnabledCPU(int my_cpu);
int GetMinimumViewWidth();
bool LoadInDeskbar();
void Usage();

class PulseApp : public BApplication {
	public:
		PulseApp(int argc, char **argv);
		~PulseApp();
		
		Prefs *prefs;

	private:
		void BuildPulse();
};

#endif
