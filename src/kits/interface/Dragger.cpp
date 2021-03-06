//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Dragger.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BDragger represents a replicant "handle".
//------------------------------------------------------------------------------

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Dragger.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Shelf.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Globals ---------------------------------------------------------------------
bool BDragger::sVisible;
bool BDragger::sInited;
BLocker BDragger::sLock;
BList BDragger::sList;

static rgb_color kZombieColor = {220, 220, 220, 255};

const unsigned char kHandBitmap[] = {	255, 255,   0,   0,   0, 255, 255, 255,
										255, 255,   0, 131, 131,   0, 255, 255,
										  0,   0,   0,   0, 131, 131,   0,   0,
										  0, 131,   0,   0, 131, 131,   0,   0,
										  0, 131, 131, 131, 131, 131,   0,   0,
										255,   0, 131, 131, 131, 131,   0,   0,
										255, 255,   0,   0,   0,   0,   0,   0,
										255, 255, 255, 255, 255, 255,   0,   0};


BDragger::BDragger(BRect bounds, BView *target, uint32 rmask, uint32 flags)
	:	BView(bounds, "_dragger_", rmask, flags),
		fTarget(target),
		fRelation(TARGET_UNKNOWN),
		fShelf(NULL),
		fTransition(false),
		fIsZombie(false),
		fErrCount(0),
		fPopUp(NULL)
{
	fBitmap = new BBitmap(BRect(0.0f, 0.0f, 7.0f, 7.0f), B_CMAP8, false, false);
	fBitmap->SetBits(kHandBitmap, fBitmap->BitsLength(), 0, B_CMAP8);
}


BDragger::BDragger(BMessage *data)
	:	BView(data),
		fTarget(NULL),
		fRelation(TARGET_UNKNOWN),
		fShelf(NULL),
		fTransition(false),
		fIsZombie(false),
		fErrCount(0),
		fPopUp(NULL)
{
	data->FindInt32("_rel", (int32*)&fRelation);

	fBitmap = new BBitmap(BRect(0.0f, 0.0f, 7.0f, 7.0f), B_CMAP8, false, false);
	fBitmap->SetBits(kHandBitmap, fBitmap->BitsLength(), 0, B_CMAP8);

	BMessage popupMsg;

	if (data->FindMessage("_popup", &popupMsg) == B_OK) {
		BArchivable *archivable = instantiate_object(&popupMsg);

		if (archivable)
			fPopUp = dynamic_cast<BPopUpMenu*>(archivable);
	}
}


BDragger::~BDragger()
{
	SetPopUp(NULL);
	
	delete fBitmap;
}


BArchivable	*
BDragger::Instantiate(BMessage *data)
{
	if (validate_instantiation(data, "BDragger"))
		return new BDragger(data);
	else
		return NULL;
}


status_t 
BDragger::Archive(BMessage *data, bool deep) const
{
	BMessage popupMsg;

	if (fPopUp) {
		fPopUp->Archive(&popupMsg);
		data->AddMessage("_popup", &popupMsg);
	}

	data->AddInt32("_rel", fRelation);

	return BView::Archive(data, deep);
}


void
BDragger::AttachedToWindow() 
{
	if (fIsZombie) {
		SetLowColor(kZombieColor);
		SetViewColor(kZombieColor);
	
	} else {
		SetLowColor(B_TRANSPARENT_COLOR);
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	determine_relationship();
	ListManage(true);
}


void
BDragger::DetachedFromWindow()
{
	ListManage(false);
}


void
BDragger::Draw(BRect update)
{
	BRect bounds(Bounds());

	if (AreDraggersDrawn() && (!fShelf || fShelf->AllowsDragging())) {
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fBitmap, bounds.LeftTop());
		SetDrawingMode(B_OP_COPY);

		if (fIsZombie) {
			// TODO: should draw it differently ?
		}
	} else if (IsVisibilityChanging()) {
		if (Parent())
			Parent()->Invalidate(Frame());
		
		else {
			SetHighColor(255, 255, 255);
			FillRect(bounds);
		}
	}
}


void
BDragger::MouseDown(BPoint where)
{
	if (!fTarget || !AreDraggersDrawn())
		return;

	int32 buttons;

	Window()->CurrentMessage()->FindInt32("buttons", &buttons);

	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		if (!fShelf || !fTarget) {
			beep();
			return;
		}

		ShowPopUp(fTarget, where);
	
	} else {
		// TODO code to determine drag or menu
		/*bigtime_t time = system_time();
		bigtime_t click_speed = 0;

		get_click_speed(&click_speed);*/

		bool drag = false;

		while (true) {
			BPoint where2;
			uint32 buttons;

			GetMouse(&where2, &buttons);
			
			if (!buttons)
				break;
			
			if (where2 != where) {
				drag = true;
				break;
			}

			snooze(40000);
		}

		if (drag) {
			BMessage archive(B_ARCHIVED_OBJECT);

			if (fRelation == TARGET_IS_PARENT)
				fTarget->Archive(&archive);
			else if (fRelation == TARGET_IS_CHILD)
				Archive(&archive);
			else {
				if (fTarget->Archive(&archive)) {
					BMessage widget(B_ARCHIVED_OBJECT);

					if (Archive(&widget))
						archive.AddMessage("__widget", &widget);
				}
			}

			archive.AddInt32("be:actions", B_TRASH_TARGET);

			
			BPoint offset;
			drawing_mode mode;		
			BBitmap *bitmap = DragBitmap(&offset, &mode);
			if (bitmap)
				DragMessage(&archive, bitmap, mode, offset, this);
			else
				DragMessage(&archive,
					ConvertFromScreen(fTarget->ConvertToScreen(fTarget->Bounds())),
					this);
		} else
			ShowPopUp(fTarget, where);
	}
}


void
BDragger::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}


void
BDragger::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}


void
BDragger::MessageReceived(BMessage *msg)
{
	if (msg->what == B_TRASH_TARGET) {
		if (fShelf)
			Window()->PostMessage('JAHA', fTarget, NULL);
		else
			(new BAlert("??",
				"Can't delete this replicant from its original application. Life goes on.",
				"OK", NULL, NULL, B_WIDTH_FROM_WIDEST, B_WARNING_ALERT))->Go(NULL);
	
	} else if (msg->what == B_SCREEN_CHANGED) {
		if (fRelation == TARGET_IS_CHILD) {
			fTransition = true;
			Draw(BRect());
			fTransition = false;
		
		} else {
			if ((fShelf && (fShelf->AllowsDragging() && AreDraggersDrawn()))
				|| AreDraggersDrawn())
				Show();
			else
				Hide();
		}
	} else
		BView::MessageReceived(msg);
}


void
BDragger::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}


void
BDragger::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}


status_t
BDragger::ShowAllDraggers()
{
	// TODO: Implement. Should ask the registrar or the app server
	return B_OK;
}


status_t
BDragger::HideAllDraggers()
{
	// TODO: Implement. Should ask the registrar or the app server
	return B_OK;
}


bool
BDragger::AreDraggersDrawn()
{
	// TODO: Implement. Should ask the registrar or the app server
	return true;
}


BHandler *BDragger::ResolveSpecifier(BMessage *msg, int32 index,
									 BMessage *specifier, int32 form,
									 const char *property)
{
	return BView::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
BDragger::GetSupportedSuites(BMessage *data)
{
	return GetSupportedSuites(data);
}


status_t
BDragger::Perform(perform_code d, void *arg)
{
	return Perform(d, arg);
}


void
BDragger::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BDragger::GetPreferredSize(float *width, float *height)
{
	BView::GetPreferredSize(width, height);
}


void
BDragger::MakeFocus(bool state)
{
	BView::MakeFocus(state);
}


void
BDragger::AllAttached()
{
	BView::AllAttached();
}


void
BDragger::AllDetached()
{
	BView::AllDetached();
}


status_t
BDragger::SetPopUp(BPopUpMenu *context_menu)
{
	if (fPopUp && fPopUp != context_menu)
		delete fPopUp;

	fPopUp = context_menu;
	return B_OK;
}


BPopUpMenu *
BDragger::PopUp() const
{
	if (!fPopUp && fTarget)
		const_cast<BDragger*>(this)->BuildDefaultPopUp();
	
	return fPopUp;
}


bool
BDragger::InShelf() const
{
	return fShelf != NULL;
}


BView *
BDragger::Target() const
{
	return fTarget;
}


BBitmap *
BDragger::DragBitmap(BPoint *offset, drawing_mode *mode)
{
	return NULL;
}


bool
BDragger::IsVisibilityChanging() const
{
	return fTransition;
}


void BDragger::_ReservedDragger2() {}
void BDragger::_ReservedDragger3() {}
void BDragger::_ReservedDragger4() {}


BDragger &
BDragger::operator=(const BDragger &)
{
	return *this;
}


void
BDragger::ListManage(bool add)
{
	if (sLock.Lock()) {
		bool drawn = AreDraggersDrawn();

		if (add) {
			bool dragging = true;

			sList.AddItem(this);

			if (fShelf)
				dragging = fShelf->AllowsDragging();

			if (!drawn && !dragging) {
				if (fRelation != TARGET_IS_CHILD)
					Hide();
			}
		} else
			sList.RemoveItem(this);

		sLock.Unlock();
	}
}


status_t
BDragger::determine_relationship()
{
	status_t err = B_OK;

	if (fTarget) {
		if (fTarget == Parent())
			fRelation = TARGET_IS_PARENT;
		else if (fTarget == ChildAt(0))
			fRelation = TARGET_IS_CHILD;
		else
			fRelation = TARGET_IS_SIBLING;
	} else {
		if (fRelation == TARGET_IS_PARENT)
			fTarget = Parent();
		else if (fRelation == TARGET_IS_CHILD)
			fTarget = ChildAt(0);
		else
			err = B_ERROR;
	}

	return err;
}


status_t
BDragger::SetViewToDrag(BView *target)
{
	if (target->Window() != Window())
		return B_ERROR;

	fTarget = target;

	if (Window())
		determine_relationship();

	return B_OK;
}


void
BDragger::SetShelf(BShelf *shelf)
{
	fShelf = shelf;
}


void
BDragger::SetZombied(bool state)
{
	fIsZombie = state;

	SetLowColor(kZombieColor);
	SetViewColor(kZombieColor);
}


void
BDragger::BuildDefaultPopUp()
{
	fPopUp = new BPopUpMenu("Shelf", false, false, B_ITEMS_IN_COLUMN);

	// About
	BMessage *msg = new BMessage(B_ABOUT_REQUESTED);

	const char *name = fTarget->Name();

	if (name)
		msg->AddString("target", name);

	// TODO: Fix this
	char *about = (char*)malloc(6 + (name ? strlen(name) : 0) + 1);
	sprintf(about, "About %s", name);

	fPopUp->AddItem(new BMenuItem(about, msg));

	free(about);

	// Seperator
	fPopUp->AddItem(new BSeparatorItem());

	// Delete
	msg = new BMessage('JAHA');

	fPopUp->AddItem(new BMenuItem("Delete", msg));
}


void
BDragger::ShowPopUp(BView *target, BPoint where)
{
	BPoint point = ConvertToScreen(where);

	if (!fPopUp && fTarget)
		BuildDefaultPopUp();

	fPopUp->SetTargetForItems(fTarget);

	fPopUp->Go(point, true, false, /*BRect(), */true);
}
