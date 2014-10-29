/*This is diswindow.cpp*/

#include <Application.h>
#include "diswindow.h"
#include "disview.h"
#include "discordian.h"

#include <iostream>
#include <String.h>


DisWindow::DisWindow (BRect aRect)
	: BWindow ( aRect,"Discordian Calendar", B_TITLED_WINDOW, B_NOT_V_RESIZABLE) 
									// make a window (DisWindow) based on BWindow class 

{														// make this stuff see AddChild Below
	
	str Astring = "";							// Gimmie a new empty string
	DisView *pMyView;						// Gimmie a new pointer to  a view called pMyView - I put p in 
														// front of a variable to designate that it is a pointer
	juletodis fluffy;
	
	fluffy.todis(Astring);						// dump the return of Fluffy subroutine todis to Astring
	BRect bRect ( Bounds());				// Gimmie a rect based on the window's bounds
	pMyView = new DisView(bRect, "aView", Astring);		// Make a view with aRect borders, called aView, 
														// Dump Astring into it	
	
	AddChild( pMyView );					// Attach the above stuff to the window					

	
	return;
}

bool DisWindow :: QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);	// if you say Quit, Quit
	return (true);
}
