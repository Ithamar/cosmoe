/*This is Disview.h*/
#ifndef DIS_VIEW_H
#define DIS_VIEW_H

#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif



class DisView : public BStringView
{
public:
	DisView (BRect aRect, const char *name, const char *text);
	virtual ~DisView(){};
};

#endif
