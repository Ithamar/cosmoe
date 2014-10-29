/* This is DisApp.cpp*/

#include "disapp.h"
#include "diswindow.h"
#include "disview.h"

#include "julian.h"
#include "discordian.h"


DisApplication::DisApplication()
	: BApplication ("application/x-vnd.dps-discordapp")	// Make a new application bassed on the BApplication class
{
	DisWindow *pDwindow;						// Gimmie a window to put it in
	BRect aRect;										// Gimmie a BRect that defines the window

	aRect.Set(30,100,640,125);					// Left, Top, Right and Bottom borders of the Window
	pDwindow = new DisWindow (aRect);	// Gimmie a new window bassed on DisWindow

	pDwindow->Show();							// Show me da Window!
}
