/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include <iostream>

#include <Window.h>
#include <Application.h>
#include <Beep.h>
#include <Screen.h>

#include "Calculator.h"
#include "CalcWindow.h"



/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
int main(int argc, char **argv)
{
	CalculatorApp app;
	app.Run();

	return B_NO_ERROR;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
CalculatorApp::CalculatorApp()
		  		  : BApplication("application/x-vnd.Cosmoecalc")
{
	wind = new CalcWindow(BPoint(20.0f, 70.0f));
	wind->Show();
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
void CalculatorApp::AboutRequested()
{
	CalcView::About();
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
