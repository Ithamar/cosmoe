/* This is DisApp.cpp*/

#include "disapp.h"
#include "diswindow.h"


DisApplication::DisApplication()
	: BApplication ("application/x-vnd.Guido")	// Make a new application bassed on the BApplication class
{
	DisWindow *pDwindow;					// Gimmie a window to put it in
	BRect aRect;							// Gimmie a BRect that defines the window

	aRect.Set(30,100,440,400);				// Left, Top, Right and Bottom borders of the Window
	pDwindow = new DisWindow (aRect);		// Gimmie a new window based on DisWindow
	pDwindow->Populate();
	pDwindow->Show();						// Show me da Window!
}
