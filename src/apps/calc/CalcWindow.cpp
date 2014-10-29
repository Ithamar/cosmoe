/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "Calculator.h"
#include "CalcWindow.h"
#include "CalcView.h"



/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
CalcWindow::CalcWindow(BPoint pt)
		: BWindow(CalcView::TheRect(pt), "Calculator",
				  B_TITLED_WINDOW, B_NOT_H_RESIZABLE)
{
	BRect r = CalcView::TheRect();

	SetSizeLimits(r.right, r.right, r.bottom, r.bottom+CalcView::ExtraHeight());
	//SetZoomLimits(r.right, r.bottom+CalcView::ExtraHeight());

	calc_view = new CalcView(r);
	AddChild(calc_view);

	BView *extraView = calc_view->AddExtra();
	AddChild(extraView);
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
bool CalcWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
