
#ifndef DIS_VIEW_H	
#include "disview.h"	
#endif				

#include <Font.h>

DisView::DisView(BRect aRect,
		 const char *name,
		 const char *text)
					: BStringView ( aRect,
							name,
							text,
							B_FOLLOW_ALL_SIDES,
							B_WILL_DRAW)
	   	   // Gimmie a view bassed on BStringView
{
	//SetFont(be_bold_font);			// set font
	SetFontSize(16);			// set size

}

