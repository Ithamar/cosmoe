/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Debug.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>

#include <fs_index.h>
#include <fs_info.h>

#include <Debug.h>
#include <Application.h>
#include <Beep.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "DeskBarUtils.h"
#include "StatusView.h"
#include "StatusViewShelf.h"
#include "TimeView.h"
#include "BarApp.h"

#ifdef DB_ADDONS
//	Add-on support
//
//	Item - internal item list (node, eref, etc)
//	Icon - physical replicant handed to the DeskbarClass class
//	AddOn - attribute based add-on

const char *const kInstantiateItemCFunctionName = "instantiate_deskbar_item";
const char *const kInstantiateEntryCFunctionName = "instantiate_deskbar_entry";
const char *const kDeskbarSecurityCodeFile = "Deskbar_security_code";
const char *const kDeskbarSecurityCodeAttr = "be:deskbar_security_code";
const char *const kStatusPredicate = "be:deskbar_item_status";
const char *const kEnabledPredicate = "be:deskbar_item_status=enabled";
const char *const kDisabledPredicate = "be:deskbar_item_status=disabled";


static void
DumpItem(DeskbarItemInfo *item)
{
	printf("is addon: %i, id: %li\n", item->isaddon, item->id);
	printf("entry_ref:  %ld, %Ld, %s\n", item->edevice, item->edirectory, item->ename);
	printf("node_ref:  %ld, %Ld\n", item->nref.device, item->nref.node);
	printf("real dev:  %li\n", item->realDevice);				
}

static void
DumpList(BList *itemlist)
{
	int32 count = itemlist->CountItems()-1;
	if (count < 0) {
		printf("no items in list\n");
		return;
	}
	for (int32 i=count ; i>=0 ; i--) {
		DeskbarItemInfo *item = (DeskbarItemInfo*)itemlist->ItemAt(i);
		if (!item)
			continue;
			
		DumpItem(item);
	}
}
#endif	/* DB_ADDONS */


//	don't change the name of this view to anything other than
//	Status 
TReplicantTray::TReplicantTray(TBarView *parent, bool vertical)
	:	BView(BRect(0,0,1,1), "Status", B_FOLLOW_LEFT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FRAME_EVENTS),
		fClock(NULL),
		fBarView(parent),
		fShelf(new TReplicantShelf(this)),
		fMultiRowMode(vertical),
		fAlignmentSupport(false)
{
}

TReplicantTray::~TReplicantTray()
{
	delete fShelf;
}

void
TReplicantTray::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetViewColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_DARKEN_1_TINT));
	SetDrawingMode(B_OP_COPY);
	Window()->SetPulseRate(1000000);
	DealWithClock(fBarView->ShowingClock());	

#ifdef DB_ADDONS		
	//	load addons and rehydrate archives		
	InitAddOnSupport();	
#endif
	ResizeToPreferred();
}

void
TReplicantTray::DetachedFromWindow()
{
#ifdef DB_ADDONS
	//	clean up add-on support
	DeleteAddOnSupport();
#endif	
	BView::DetachedFromWindow();
}

void
TReplicantTray::RememberClockSettings()
{
	if (fClock)	{
		desk_settings *settings = ((TBarApp *)be_app)->Settings();

		settings->timeShowSeconds = fClock->ShowingSeconds();
		settings->timeShowMil = fClock->ShowingMilTime();
		settings->timeShowEuro = fClock->ShowingEuroDate();
		settings->timeFullDate = fClock->ShowingFullDate();
	}
}

bool
TReplicantTray::ShowingSeconds()
{
	if (fClock)
		return fClock->ShowingSeconds();
	return false;
}

bool
TReplicantTray::ShowingMiltime()
{
	if (fClock)
		return fClock->ShowingMilTime();
	return false;
}

bool
TReplicantTray::ShowingEuroDate()
{
	if (fClock)
		return fClock->ShowingEuroDate();
	return false;
}

bool
TReplicantTray::ShowingFullDate()
{
	if (fClock)
		return fClock->ShowingFullDate();
	return false;
}

bool
TReplicantTray::CanShowFullDate()
{
	if (fClock)
		return fClock->CanShowFullDate();
	return false;
}

void
TReplicantTray::DealWithClock(bool showClock)
{
	if (showClock) {
		if (!fClock) {
			desk_settings *settings = ((TBarApp *)be_app)->Settings();

			fClock = new TTimeView(settings->timeShowSeconds, settings->timeShowMil,
				settings->timeFullDate, settings->timeShowEuro, false);
			AddChild(fClock);
			fClock->MoveTo(Bounds().right - fClock->Bounds().Width() - 1, 2);
		}
	} else {
		if (fClock) {
			RememberClockSettings();

			fClock->RemoveSelf();
			delete fClock;
			fClock = NULL;
		}
	}
	fBarView->ShowClock(showClock);
}

//	width is set to a minimum of kMinimumReplicantCount by kMaxReplicantWidth
// 	if not in multirowmode and greater than kMinimumReplicantCount
//	the width should be calculated based on the actual
//	replicant widths
void
TReplicantTray::GetPreferredSize(float *preferredWidth, float *preferredHeight)
{
	float width = 0, height = kMinimumTrayHeight;
	
	uint32 id;
	BView *view;
	fShelf->ReplicantAt(IconCount() - 1, &view, &id);
	if (fMultiRowMode) {
		if (view)
			height = view->Frame().bottom;

		// 	the height will be uniform for the number of rows
		//	necessary to show all the reps + any gutters
		//	necessary for spacing	
		int32 rowCount = (int32)(height / kMaxReplicantHeight);
		height = kGutter + (rowCount * kMaxReplicantHeight)
			+ ((rowCount - 1) * kIconGap) + kGutter;
		// if new replicant budges into clock's area on clock row, add a row
		if (view && fBarView->ShowingClock()
				&& view->Frame().right >= fClock->Frame().left
				&& view->Frame().top == fClock->Frame().top)
				height += kMaxReplicantHeight;
		height = max_c(kMinimumTrayHeight, height);
		width = kMinimumTrayWidth;
	} else {
		// if last replicant overruns clock then
		// resize to accomodate
		if (view) {	
			BRect viewFrame(view->Frame());	
			if (fBarView->ShowingClock()
				&& viewFrame.right + 6 >= fClock->Frame().left) {
				width = viewFrame.right + 6 + fClock->Frame().Width();
			} else 
				width = viewFrame.right + 3;
		}
		//	this view has a fixed minimum width
		width = max_c(kMinimumTrayWidth, width);
	}

	*preferredWidth = width;
	//	add 2 for the border
	*preferredHeight = height + 1;
}

void
TReplicantTray::AdjustPlacement()
{
	//	called when an add-on has been added or removed
	//	need to resize the parent of this accordingly
	//		
	//	call to Parent will call ResizeToPreferred
	Parent()->ResizeToPreferred();
	fBarView->UpdatePlacement();
	Parent()->Invalidate();
	Invalidate();
}

void
TReplicantTray::Draw(BRect)
{
	rgb_color menuColor = ui_color(B_MENU_BACKGROUND_COLOR);
	rgb_color vdark = tint_color(menuColor, B_DARKEN_3_TINT);
	rgb_color light = tint_color(menuColor, B_LIGHTEN_2_TINT);
	
	BRect frame(Bounds());

	SetHighColor(light);
	StrokeLine(frame.LeftBottom(), frame.RightBottom());
	StrokeLine(frame.RightBottom(), frame.RightTop());
	
	SetHighColor(vdark);
	StrokeLine(frame.RightTop(), frame.LeftTop());
	StrokeLine(frame.LeftTop(), frame.LeftBottom());
}

void
TReplicantTray::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case 'time':
			// from context menu in clock and in this view
			DealWithClock(!fBarView->ShowingClock());
			RealignReplicants();
			AdjustPlacement();
			break;
		
		case 'trfm':
			// time string reformat -> realign
			DealWithClock(fBarView->ShowingClock());
			RealignReplicants();
			AdjustPlacement();
			break;
		
		case msg_showseconds:
			if (fClock) 
				fClock->ShowSeconds(!fClock->ShowingSeconds());
			break;
		
		case msg_miltime:
			if (fClock) 
				fClock->ShowMilTime(!fClock->ShowingMilTime());
			break;
		
		case msg_eurodate:
			if (fClock)
				fClock->ShowEuroDate(!fClock->ShowingEuroDate());
			break;
		
		case msg_fulldate:
			if (fClock)
				if (fClock->CanShowFullDate())
					fClock->ShowFullDate(!fClock->ShowingFullDate());
			break;

#ifdef DB_ADDONS
		case B_NODE_MONITOR:
		case B_QUERY_UPDATE:
			HandleEntryUpdate(message);
			break;
#endif
						
		default:
			BView::MessageReceived(message);
			break;
	}
}

void
TReplicantTray::ShowReplicantMenu(BPoint point)
{
	BPopUpMenu *menu = new BPopUpMenu("", false, false);
//	menu->SetFont(be_plain_font);

	// If the clock is visible, show the extended menu
	// otheriwse, show "Show Time".
	
	if (fBarView->ShowingClock())
		fClock->ShowClockOptions(ConvertToScreen(point));
	else {
		BMenuItem *item = new BMenuItem("Show Time", new BMessage('time'));
		menu->AddItem(item);
		menu->SetTargetForItems(this);
		BPoint where = ConvertToScreen(point);
		menu->Go(where, true, true, BRect(where - BPoint(4, 4), 
			where + BPoint(4, 4)), true);
	}
}

void
TReplicantTray::MouseDown(BPoint where)
{
#ifdef DB_ADDONS
	if (modifiers() & B_CONTROL_KEY) 
		DumpList(fItemList);
#endif

	uint32	buttons;

	Window()->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowReplicantMenu(where);
	} else {
		BPoint save = where;
		bigtime_t doubleClickSpeed;
		bigtime_t start = system_time();
		uint32 buttons;

		get_click_speed(&doubleClickSpeed);

		do {
			if (fabs(where.x - save.x) > 4 || fabs(where.y - save.y) > 4)
				//	user moved out of bounds of click area
				break;
	
			if ((system_time() - start) > (2 * doubleClickSpeed)) {
				ShowReplicantMenu(where);
				break;
			}
	
			snooze(50000);
			GetMouse(&where, &buttons);
		} while (buttons);
	}
	BView::MouseDown(where);
}

#ifdef DB_ADDONS		

void
TReplicantTray::InitAddOnSupport()
{
	// list to maintain refs to each rep added/deleted
	fItemList = new BList();

	bool haveKey = false;
 	BPath path;
    if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		path.Append(kDeskbarSecurityCodeFile);
		BFile file(path.Path(),B_READ_ONLY);
		if (file.InitCheck() == B_OK
			&& file.Read(&fDeskbarSecurityCode,
				sizeof(fDeskbarSecurityCode)) == sizeof(fDeskbarSecurityCode))
			haveKey = true;
		}
	if (!haveKey) {
		// create the security code
		bigtime_t real = real_time_clock_usecs();
		bigtime_t boot = system_time();
		// two computers would have to have exactly matching clocks, and launch
		// Deskbar at the exact same time into the bootsequence in order for their
		// security-ID to be identical
		fDeskbarSecurityCode = ((real&0xffffffffULL)<<32)|(boot&0xffffffffULL);
    	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
			path.Append(kDeskbarSecurityCodeFile);
			BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
			if (file.InitCheck() == B_OK)
				file.Write(&fDeskbarSecurityCode, sizeof(fDeskbarSecurityCode));
		}
	}
 

	
	//	for each volume currently mounted
	//		index the volume with our indices
	BVolumeRoster roster;
	BVolume volume;
	while (roster.GetNextVolume(&volume) == B_OK) {

		fs_create_index(volume.Device(), kStatusPredicate, B_STRING_TYPE, 0);
		RunAddOnQuery(&volume, kEnabledPredicate);
	}

	//	we also watch for volumes mounted and unmounted
	watch_node(NULL, B_WATCH_MOUNT | B_WATCH_ATTR, this, Window());
}

void
TReplicantTray::DeleteAddOnSupport()
{
	int32 count = fItemList->CountItems();
	for (int32 i=count ; i >= 0 ; i--) {
		DeskbarItemInfo *item = (DeskbarItemInfo *)fItemList->RemoveItem(i);
		if (item) {
			if (item->isaddon)
				watch_node(&(item->nref), B_STOP_WATCHING, this, Window());
			free(item->ename);		//strdup'd in LoadAddOn
			free(item);
		}
	}
	delete fItemList;

	//	stop the volume mount/unmount watch
	stop_watching(this, Window());
}

void
TReplicantTray::RunAddOnQuery(BVolume *volume, const char *predicate)
{
	// Since the new BFS supports querying for attributes without
	// an index, we only run the query if the index exists (for
	// newly mounted devices only - the Deskbar will automatically
	// create an index for every device mounted at startup).
	index_info info;
	if (!volume->KnowsQuery() || fs_stat_index(volume->Device(),kStatusPredicate,&info) != 0)
		return;

	//	run a new query on a specific volume
	//	make it live
	BQuery query;
	query.SetVolume(volume);
	query.SetPredicate(predicate);
	query.Fetch();

	int32 id;
	BEntry entry;
	while (query.GetNextEntry(&entry) == B_OK) 
		//	scan any entries returned
		//	attempt to load them as add-ons
		//	collisions are handled in LoadAddOn
		LoadAddOn(&entry, &id);
}

bool
TReplicantTray::IsAddOn(entry_ref *eref)
{
	BFile file(eref, O_RDWR);
	char buffer[128];
	ssize_t size = file.ReadAttr(kStatusPredicate, B_STRING_TYPE,
		0, &buffer, 128);
		
	return size > 0;
}

bool
TReplicantTray::NodeExists(node_ref *nodeRef)
{
	int32 count = fItemList->CountItems()-1;	
	for (int32 i=count ; i >= 0 ; i--) {
		DeskbarItemInfo *item = (DeskbarItemInfo *)fItemList->ItemAt(i);
		if (!item)
			continue;
			
		if (item->nref == *nodeRef)
			return true;
	}
	return false;
}

//	received from B_NODE_MONITOR & B_QUERY_UPDATE msgs
void
TReplicantTray::HandleEntryUpdate(BMessage *message)
{
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK)
		return;
		
	BPath path;
	switch (opcode) {
		case B_ENTRY_CREATED:
			//	entry was just listed, matches live query
			{
				const char *name;
				ino_t directory;
				dev_t device;
				// 	received when an app adds a ref to the
				//	Deskbar add-ons folder
				if (message->FindString("name", &name) == B_OK
					&& message->FindInt64("directory", (int64*)&directory) == B_OK
					&& message->FindInt32("device", (int32*)&device) == B_OK) {
					entry_ref ref(device, directory, name);
					//	see if this item has the attribute
					//	that we expect
					if (IsAddOn(&ref)) {
						int32 id;
						BEntry entry(&ref);					
						LoadAddOn(&entry, &id);
					}
				}
			}
			break;

		case B_ATTR_CHANGED:
			//	from node watch on individual items
			//		(node_watch added in LoadAddOn)
			{
				node_ref nodeRef;
				if (message->FindInt32("device", (int32*)&(nodeRef.device)) == B_OK
					&& message->FindInt64("node", (int64*)&(nodeRef.node)) == B_OK) {
					int32 count = fItemList->CountItems()-1;	
					for (int32 i = count ; i >= 0 ; i--) {
						DeskbarItemInfo *item = (DeskbarItemInfo*)fItemList->ItemAt(i);
						if (!item)
							continue;

						//	match both device and inode
						if (item->nref == nodeRef) {
							entry_ref ref(item->edevice, item->edirectory, item->ename);
							BFile file(&ref, O_RDWR);
							
							char str[255];
							ssize_t size = file.ReadAttr(kStatusPredicate,
								B_STRING_TYPE, 0, str, 255);

							//	attribute was removed
							if (size == B_ENTRY_NOT_FOUND) {

								//	cleans up and removes node_watch
								UnloadAddOn(&nodeRef, NULL, true, false);
								break;
							} else if (str && strcmp(str, "enable") == 0) {
								int32 id;
								BEntry entry(&ref, true);
								LoadAddOn(&entry, &id);
								break;
							}
						}
					}
				}
			}
			break;
	
		case B_ENTRY_MOVED:
			{
				entry_ref ref;
				ino_t todirectory;
				ino_t node;
				const char *name;
				if (message->FindString("name", &name) == B_OK
					&& message->FindInt64("from directory", (int64*)&(ref.directory)) == B_OK
					&& message->FindInt64("to directory", (int64*)&todirectory) == B_OK
					&& message->FindInt32("device", (int32*)&(ref.device)) == B_OK
					&& message->FindInt64("node", (int64*)&node) == B_OK ) {
					
					if (!name)
						break;
						
					ref.set_name(name);
					//	change the directory reference to
					//	the new directory
					MoveItem(&ref, todirectory, node);
				}
			}
			break;
			
		case B_ENTRY_REMOVED:
			{
				//	entry was rm'd from the device
				node_ref nodeRef;
				if (message->FindInt32("device", (int32*)&(nodeRef.device)) == B_OK
					&& message->FindInt64("node", (int64*)&(nodeRef.node)) == B_OK ) {
					UnloadAddOn(&nodeRef, NULL, true, false);
				}
			}
			break;
				
		case B_DEVICE_MOUNTED:
			{
				//	run a new query on the new device
				dev_t device;
				if (message->FindInt32("new device", (int32*)&device) != B_OK)
					break;

				RunAddOnQuery(new BVolume(device), kEnabledPredicate);
			}
			break;
		case B_DEVICE_UNMOUNTED:
			{
				//	remove all items associated with the device
				//	unmounted
				//  contrary to what the BeBook says, the item is called "device",
				//  not "new device" like it is for B_DEVICE_MOUNTED
				dev_t device;
				if (message->FindInt32("device", (int32*)&device) != B_OK)
					break;
				
				UnloadAddOn(NULL, &device, false, true);	
			}
			break;
	}	
}

//	the add-ons must support the exported C function API
//	if they do, they will be loaded and added to deskbar
//	primary function is the Instantiate function
status_t
TReplicantTray::LoadAddOn(BEntry *entry, int32 *id, bool force)
{
	if (!entry)
		return B_ERROR;

	node_ref nodeRef;
	entry->GetNodeRef(&nodeRef);
	//	no duplicates
	if (NodeExists(&nodeRef))
		return B_ERROR;

	BNode node(entry);
	if (!force) {
		status_t error = node.InitCheck();
		if (error != B_OK)
			return error;

		uint64 deskbarID;
		ssize_t size = node.ReadAttr(kDeskbarSecurityCodeAttr, B_UINT64_TYPE, 0,
			&deskbarID, sizeof(fDeskbarSecurityCode));
		if (size != sizeof(fDeskbarSecurityCode) || deskbarID != fDeskbarSecurityCode) {
			// no code or code doesn't match
			return B_ERROR;
		}
	}
	
	BPath path;
	entry->GetPath(&path);
#ifdef NOT_IN_COSMOE
	image_id addOnImageID;

	//	load the add-on
	addOnImageID = load_add_on(path.Path());
	if (addOnImageID < 0)
		return (status_t)addOnImageID;
	
	// 	get the view loading function symbol		
	//    we first look for a symbol that takes an image_id
	//    and entry_ref pointer, if not found, go with normal
	//    instantiate function
	BView *(*entryFunction)(image_id, const entry_ref *);
	BView *(*itemFunction)(void);

	BView *view = NULL;
	entry_ref ref;

	entry->GetRef (&ref);

	if (get_image_symbol(addOnImageID, kInstantiateEntryCFunctionName,
		B_SYMBOL_TYPE_TEXT, (void **)&entryFunction) >= 0) {

		view = (*entryFunction)(addOnImageID, &ref);
	} else if (get_image_symbol(addOnImageID, kInstantiateItemCFunctionName,
		B_SYMBOL_TYPE_TEXT, (void **)&itemFunction) >= 0) {

		view = (*itemFunction)();
	} else {
		unload_add_on(addOnImageID);
		return B_ERROR;
	}

	if (!view || IconExists (view->Name())) {
		delete view;
		unload_add_on (addOnImageID);
		return B_ERROR;
	}

	BMessage *data = new BMessage;
	view->Archive(data);
	delete view;

	AddIcon(data, id, &ref);
		// add the rep; adds info to list

	node.WriteAttr(kDeskbarSecurityCodeAttr, B_UINT64_TYPE, 0,
		&fDeskbarSecurityCode, sizeof(fDeskbarSecurityCode));
#endif
	return B_OK;
}

status_t
TReplicantTray::AddItem(int32 id, node_ref nodeRef, BEntry *entry, bool isaddon)
{
	DeskbarItemInfo *item = (DeskbarItemInfo*)malloc(sizeof(DeskbarItemInfo));
	if (!item)
		return B_ERROR;

	item->id = id;
	item->isaddon = isaddon;
	
	entry_ref ref;			
	entry->GetRef(&ref);
	
	item->edevice = ref.device;
	item->edirectory = ref.directory;
	if (ref.name && strlen(ref.name)>0)
		item->ename = strdup(ref.name);
	else
		item->ename = strdup("");			// The pointer can't be null
	
	item->nref = nodeRef;
	
	//	get the actual entry
	//	since this may not be the boot volume
	//	we need the actual dev_t for matching
	//	if the volume is unmounted later
	BEntry realEntry(&ref, true);			

	realEntry.GetRef(&ref);
	item->realDevice = ref.device;

	fItemList->AddItem(item);

	if (isaddon)
		watch_node(&nodeRef, B_WATCH_NAME | B_WATCH_ATTR, this, Window());

	return B_OK;		
}

//	from entry_removed message, when attribute removed
//	or when a device is unmounted (use removeall, by device)
void
TReplicantTray::UnloadAddOn(node_ref *nodeRef, dev_t *device,
	bool which, bool removeall)
{
	int32 count = fItemList->CountItems()-1;	
	for (int32 i=count ; i>=0 ; i--) {
		DeskbarItemInfo *item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (!item)
			continue;
			
		bool found=false;
		if (which && nodeRef && item->nref == *nodeRef)
			found = true;
		else if (device && item->realDevice == *device)
			found = true;
		
		if (found) {
			RemoveIcon(item->id);
			
			if (!removeall)
				break;
		}
	}
}

void
TReplicantTray::RemoveItem(int32 id)
{
	int32 count = fItemList->CountItems() - 1;	
	for (int32 i = count ; i >= 0 ; i--) {
		DeskbarItemInfo *item = (DeskbarItemInfo*)fItemList->ItemAt(i);
		if (!item)
			continue;
			
		if (item->id == id) {
			//	attribute was added via Deskbar API (AddItem(entry_ref *, int32 *)
			entry_ref ref(item->edevice, item->edirectory, item->ename);
			if (item->isaddon) {
				BNode node(&ref);
				node.RemoveAttr(kStatusPredicate);
				watch_node(&(item->nref), B_STOP_WATCHING, this, Window());			
			}

			fItemList->RemoveItem(i);
			
			free(item->ename);
				// strdup'd in LoadAddOn
			free(item);
			
			break;
		}
	}
}

//	ENTRY_MOVED message, moving only occurs on a device
//	copying will occur (ENTRY_CREATED) between devices
void
TReplicantTray::MoveItem(entry_ref *ref, ino_t toDirectory, ino_t DEBUG_ONLY(node))
{
	if (!ref)
		return;

	//	scan for a matching entry_ref
	//
	//	don't need to change node info as it does not change
	//		- what about mv to other vol?
	int32 count = fItemList->CountItems()-1;	
	for (int32 i = count ; i >= 0 ; i--) {
		DeskbarItemInfo *item = (DeskbarItemInfo *)fItemList->ItemAt(i);
		if (!item)
			continue;
			
		if (strcmp(item->ename, ref->name) == 0
			&& item->edevice == ref->device
			&& item->edirectory == ref->directory) {
			item->edirectory = toDirectory;
			break;
		}
	}
}

#endif	//	add-on support

//	external add-on API routines
//	called using the new BDeskbar class

//	existence of icon/replicant by name or ID
//	returns opposite
//	note: name and id are semi-private limiting
//		the ability of non-host apps to remove
//		icons without a little bit of work

/**	for a specific id
 *	return the name of the replicant (name of view)
 */

status_t
TReplicantTray::ItemInfo(int32 id, const char **name)
{
	if (id < 0)
		return B_ERROR;
	
	int32 index, temp;
	BView *view = ViewAt(&index, &temp, id, false);
	if (view) {
		*name = view->Name();
		return B_OK;
	}
	
	return B_ERROR;
}

//	for a specific name
//	return the id (internal to Deskbar)
status_t
TReplicantTray::ItemInfo(const char *name, int32 *id)
{
	if (!name || strlen(name) <= 0)
		return B_ERROR;
		
	int32 index;
	BView *view = ViewAt(&index, id, name);
	if (view)
		return B_OK;
	
	return B_ERROR;
}

//	at a specific index
//	return both the name and the id of the replicant
status_t
TReplicantTray::ItemInfo(int32 index, const char **name, int32 *id)
{
	if (index < 0)
		return B_ERROR;
		
	BView *view;
	fShelf->ReplicantAt(index, &view, (uint32 *)id, NULL);
	if (view) {
		*name = view->Name();
		return B_OK;
	}
	
	return B_ERROR;
}

//	replicant exists, by id/index
bool
TReplicantTray::IconExists(int32 target, bool byIndex)
{
	int32 index, id;
	BView *view = ViewAt(&index, &id, target, byIndex);
		
	return view && index >= 0;
}

//	replicant exists, by name
bool
TReplicantTray::IconExists(const char *name)
{
	if (!name || strlen(name) == 0)
		return false;
		
	int32 index, id;
	BView *view = ViewAt(&index, &id, name);
	
	return view && index >= 0;
}

int32
TReplicantTray::IconCount() const
{
	return fShelf->CountReplicants();
}

//	message must contain an archivable view
//	in the Archives folder for later rehydration
//	returns the current boot id
status_t
TReplicantTray::AddIcon(BMessage *icon, int32 *id, const entry_ref *addOn)
{
	if (!icon || !id)
		return B_ERROR;

	*id = 999;
	if (icon->what == B_ARCHIVED_OBJECT)
		icon->what = 0;

	//	!! check for name collisions?
	status_t err = fShelf->AddReplicant(icon, BPoint(1, 1));
	if (err != B_OK)
		return err;
	
	float oldWidth = Bounds().Width();
	float oldHeight = Bounds().Height();
	float width, height;
	GetPreferredSize(&width, &height);
	if (oldWidth != width || oldHeight != height) 
		AdjustPlacement();
	
	int32 count = fShelf->CountReplicants();
	BView *view;
	fShelf->ReplicantAt(count-1, &view, (uint32 *)id, NULL);
	
	//	add the item to the add-on list
	entry_ref ref;
	if (addOn) // Use it if we got it
		ref = *addOn;
	else {
		const char *appsig;
		icon->FindString("add_on", &appsig);
		BRoster roster;
		roster.FindApp(appsig, &ref);
	}

	BFile file(&ref, O_RDWR);
	node_ref nodeRef;
	file.GetNodeRef(&nodeRef);
	BEntry entry(&ref);
	AddItem(*id, nodeRef, &entry, addOn != NULL);

 	return B_OK;
}

void
TReplicantTray::RemoveIcon(int32 target, bool byIndex)
{
	if (target < 0)
		return;
	
	int32 index, id;
	BView *view = ViewAt(&index, &id, target, byIndex);
	if (view && index >= 0) {
		//	remove the reference from the item list
		RemoveItem(id);
		//	remove the replicant from the shelf
		fShelf->DeleteReplicant(index);
		//	force a placement update,  !! need to fix BShelf
		RealReplicantAdjustment(index);
	}
}

void
TReplicantTray::RemoveIcon(const char *name)
{
	if (!name || strlen(name) <= 0)
		return;
	
	int32 id, index;
	BView *view = ViewAt(&index, &id, name);
	if (view && index >= 0) {
		//	remove the reference from the item list
		RemoveItem(id);
		//	remove the replicant from the shelf
		fShelf->DeleteReplicant(index);
		//	force a placement update,  !! need to fix BShelf
		RealReplicantAdjustment(index);
	}
}

void
TReplicantTray::RealReplicantAdjustment(int32 startindex)
{
	if (startindex < 0)
		return;
	//	reset the locations of all replicants after the one deleted
	RealignReplicants(startindex);

	float oldWidth = Bounds().Width();
	float oldHeight = Bounds().Height();
	float width, height;
	GetPreferredSize(&width, &height);
	if (oldWidth != width || oldHeight != height) {
		//	resize view to accomodate the replicants
		//	redraw as necessary
		AdjustPlacement();
	}
}

//	looking for a replicant by id/index
//	return the view and index
BView *
TReplicantTray::ViewAt(int32 *index, int32 *id, int32 target, bool byIndex)
{
	*index = -1;
	
	BView *view;
	if (byIndex){
		if (fShelf->ReplicantAt(target, &view, (uint32 *)id)) {
			if (view) {
				*index = target;
				return view;
			}
		}
	} else {
		int32 count = fShelf->CountReplicants()-1;
		int32 localid;
		for (int32 repIndex = count ; repIndex >= 0 ; repIndex--) {
			fShelf->ReplicantAt(repIndex, &view, (uint32 *)&localid);
			if (localid == target && view) {
				*index = repIndex;
				*id = localid;
				return view;
			}
		}
	}
	
	return NULL;
}

//	looking for a replicant with a view by name
//	return the view, index and the id of the replicant
BView *
TReplicantTray::ViewAt(int32 *index, int32 *id, const char *name)
{
	*index = -1;
	*id = -1;
	
	BView *view;
	int32 count = fShelf->CountReplicants()-1;
	for (int32 repIndex = count ; repIndex >= 0 ; repIndex--) {
		fShelf->ReplicantAt(repIndex, &view, (uint32 *)id);
		if (view && view->Name() && strcmp(name, view->Name()) == 0) {
			*index = repIndex;
			return view;
		}
	}
	
	return NULL;
}

// 	Shelf will call to determine where and if
//	the replicant is to be added
bool
TReplicantTray::AcceptAddon(BRect replicantFrame, BMessage *message)
{
	if (!message)
		return false;
		
	if (replicantFrame.Height() > kMaxReplicantHeight)
		return false;

	alignment align = B_ALIGN_LEFT;
	if (fAlignmentSupport && message->HasBool("deskbar:dynamic_align")) {
		if (!fBarView->Vertical())
			align = B_ALIGN_RIGHT;
		else
			align = fBarView->Left() ? B_ALIGN_LEFT : B_ALIGN_RIGHT;
	} else if (message->HasInt32("deskbar:align"))
		message->FindInt32("deskbar:align", (int32 *)&align);

	if (message->HasInt32("deskbar:private_align"))
		message->FindInt32("deskbar:private_align", (int32 *)&align);
	else
		align = B_ALIGN_LEFT;
	
	int32 count = fShelf->CountReplicants();
	BPoint loc = LocForReplicant(count+1, count, replicantFrame.Width());
	
	message->AddPoint("_pjp_loc", loc);

	return true;
}

//	based on the previous (index - 1) replicant in the list
//	calculate where the left point should be for this
//	replicant.  replicant will flow to the right on its own
BPoint
TReplicantTray::LocForReplicant(int32, int32 index, float width)
{
	BPoint loc(kIconGap + 1, kGutter + 1);

	if (index > 0) {
		//	get the last replicant added for placement reference
		BView *view = NULL;		
		fShelf->ReplicantAt((index-1), &view);
		if (view) {
			// push this rep placement past the last one
			loc.x = view->Frame().right + kIconGap+1;
			loc.y = view->Frame().top;
		}
	}

	if (fMultiRowMode) {
		// if on first row, don't cover the clock
		// if on any other row, don't go past right edge
		if ((loc.x + width + 2 >= kMinimumTrayWidth)
			|| (fBarView->ShowingClock()
				&& (loc.x + width + 6 >= kMinimumTrayWidth-fClock->Frame().Width())
				&& (loc.y == kGutter + 1))) {
			// make the vertical placement uniform
			// based on the maximum height provided for each
			// replicant
			loc.y += kMaxReplicantHeight + kIconGap;
			loc.x = kIconGap + 1;
		}
	}
	return loc;
}

BRect
TReplicantTray::IconFrame(int32 target, bool byIndex)
{
	int32 index, id;
	BView *view = ViewAt(&index, &id, target, byIndex);
	if (view)
		return view->Frame();

	return BRect(0, 0, 0, 0);
}

BRect
TReplicantTray::IconFrame(const char *name)
{
	if (!name)
		return BRect(0, 0, 0, 0);
		
	int32 id, index;
	BView *view = ViewAt(&index, &id, name);
	if (view)
		return view->Frame();
	
	return BRect(0, 0, 0, 0);
}

//	scan from the startIndex and reset the location
//	as defined in LocForReplicant
void
TReplicantTray::RealignReplicants(int32 startIndex)
{
	if (startIndex < 0)
		startIndex = 0;
		
	int32 count = fShelf->CountReplicants();
	if (count <= 0)
		return;

	BView *view=NULL;
	for (int32 i=startIndex ; i<count ; i++){
		fShelf->ReplicantAt(i, &view);
		BPoint loc = LocForReplicant(count, i, view->Frame().Width());
		if (view && (view->Frame().LeftTop() != loc)) {
			view->MoveTo(loc);
		}
	}
}

void
TReplicantTray::SetMultiRow(bool state)
{
	fMultiRowMode = state;
}


//	draggable region that is asynchronous so that
//	dragging does not block other activities
TDragRegion::TDragRegion(TBarView *parent, BView *child)
	:	BControl(BRect(0, 0, 0, 0), "", "", NULL, B_FOLLOW_NONE,
			B_WILL_DRAW | B_FRAME_EVENTS),
		fBarView(parent),
		fChild(child),
		fDragLocation(kAutoPlaceDragRegion)
{
}

void
TDragRegion::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	ResizeToPreferred();
}

void
TDragRegion::GetPreferredSize(float *width, float *height)
{
	fChild->ResizeToPreferred();
	*width = fChild->Bounds().Width();
	*height = fChild->Bounds().Height();	
	
	if (fDragLocation != kNoDragRegion) 
		*width += 7;
	else
		*width += 6;

	*height += 3;
}

void
TDragRegion::FrameMoved(BPoint)
{
	if (fBarView->Left() && fBarView->Vertical() && fDragLocation != kNoDragRegion)
		fChild->MoveTo(5,2);
	else
		fChild->MoveTo(2,2);
}

void 
TDragRegion::Draw(BRect)
{
	rgb_color menuColor = ui_color(B_MENU_BACKGROUND_COLOR);
	rgb_color hilite = tint_color(menuColor, B_DARKEN_1_TINT);
	rgb_color vdark = tint_color(menuColor, B_DARKEN_3_TINT);
	rgb_color vvdark = tint_color(menuColor, B_DARKEN_4_TINT);
	rgb_color light = tint_color(menuColor, B_LIGHTEN_2_TINT);
	
	BRect frame(Bounds());
	BeginLineArray(4);

	if (fBarView->Vertical()) {
		AddLine(frame.LeftTop(), frame.RightTop(), light);
		AddLine(frame.LeftTop(), frame.LeftBottom(), light);
		AddLine(frame.RightBottom(), frame.RightTop(), hilite);
	} else if (fBarView->AcrossTop()) {
		AddLine(frame.LeftTop()+BPoint(0, 1), frame.RightTop()+BPoint(-1, 1),
			light);
		AddLine(frame.RightTop(), frame.RightBottom(), vvdark);
		AddLine(frame.RightTop()+BPoint(-1, 2),frame.RightBottom()+BPoint(-1, -1),
			hilite);
		AddLine(frame.LeftBottom(), frame.RightBottom()+BPoint(-1, 0), hilite);
	} else if (fBarView->AcrossBottom()) {
		AddLine(frame.LeftTop()+BPoint(0, 1), frame.RightTop()+BPoint(-1, 1), light);		
		AddLine(frame.LeftBottom(), frame.RightBottom(), hilite);
		AddLine(frame.RightTop(), frame.RightBottom(), vvdark);
		AddLine(frame.RightTop()+BPoint(-1, 1),frame.RightBottom()+BPoint(-1, -1),
			hilite);
	}

	EndLineArray();
	
	if (fDragLocation != kDontDrawDragRegion || fDragLocation != kNoDragRegion)
		DrawDragRegion();
}

void
TDragRegion::DrawDragRegion()
{	
	rgb_color menuColor = ui_color(B_MENU_BACKGROUND_COLOR);
	rgb_color menuHilite = tint_color(menuColor, B_HIGHLIGHT_BACKGROUND_TINT);
	rgb_color vdark = tint_color(menuColor, B_DARKEN_3_TINT);
	rgb_color light = tint_color(menuColor, B_LIGHTEN_2_TINT);

	BRect dragRegion(DragRegion());
	
	BeginLineArray(dragRegion.IntegerHeight());
	BPoint pt = dragRegion.LeftTop() + BPoint(1,1);
	
	// Draw drag region highlighted if tracking mouse
	if (IsTracking()) {
		SetHighColor(menuHilite);
		FillRect(dragRegion);
		while (pt.y + 1 <= dragRegion.bottom) {
			AddLine(pt, pt, light);
			AddLine(pt+BPoint(1,1), pt+BPoint(1,1), vdark);
			
			pt.y += 3;
		}
	} else {
		while (pt.y + 1 <= dragRegion.bottom) {
			AddLine(pt, pt, vdark);
			AddLine(pt+BPoint(1,1), pt+BPoint(1,1), light);
			
			pt.y += 3;
		}
	}
	EndLineArray();
}

BRect
TDragRegion::DragRegion() const
{
	BRect dragRegion(Bounds());
	dragRegion.top += 2;
	dragRegion.bottom -= 2;

	bool placeOnLeft=false;
	if (fDragLocation == kAutoPlaceDragRegion) {
		if (fBarView->Vertical() && fBarView->Left())
			placeOnLeft = true;
		else
			placeOnLeft = false;
	} else if (fDragLocation == kDragRegionLeft)
		placeOnLeft = true;
	else if (fDragLocation == kDragRegionRight)
		placeOnLeft = false;
	
	if (placeOnLeft) {
		dragRegion.left += 1;
		dragRegion.right = dragRegion.left + 3;
	} else {
		dragRegion.right -= 1;
		dragRegion.left = dragRegion.right - 3;
	}
	
	return dragRegion;
}

void 
TDragRegion::MouseDown(BPoint thePoint)
{
	ulong buttons;
	BPoint where;
	BRect dragRegion(DragRegion());
	
	dragRegion.InsetBy(-2.0f, -2.0f);	// DragRegion() is designed for drawing, not clicking
	
	if (!dragRegion.Contains(thePoint))
		return;
	
	while(true) {
		GetMouse(&where, &buttons);
		if (!buttons)
			break;

		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
			fPreviousPosition = thePoint;
			SetTracking(true);
			SetMouseEventMask(B_POINTER_EVENTS,
				B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
			Invalidate(DragRegion());
			break;
		}

		snooze(25000);
	}
}

void 
TDragRegion::MouseUp(BPoint pt)
{
	if (IsTracking()) {
		SetTracking(false);
		Invalidate(DragRegion());
	} else
		BControl::MouseUp(pt);
}

bool 
TDragRegion::SwitchModeForRect(BPoint mouse, BRect rect, 
	bool newVertical, bool newLeft, bool newTop, int32 newState)
{
	if (!rect.Contains(mouse))
		// not our rect
		return false;

	if (newVertical == fBarView->Vertical()
		&& newLeft == fBarView->Left()
		&& newTop == fBarView->Top()
		&& newState == fBarView->State())
		// already in the correct mode
		return true;
		
	fBarView->ChangeState(newState, newVertical, newLeft, newTop);
		
	return true;
}

void 
TDragRegion::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	if (IsTracking()) {
		BScreen screen;
		BRect frame = screen.Frame();

		float hDivider = frame.Width() / 6;
		hDivider = (hDivider < kMinimumWindowWidth + 10.0f) ? kMinimumWindowWidth + 10.0f : hDivider;
		float miniDivider = frame.top + kMiniHeight + 10.0f;
		float vDivider = frame.Height() / 2;
#ifdef FULL_MODE
		float thirdScreen = frame.Height() / 3;
#endif		
		BRect topLeft(frame.left, frame.top, frame.left + hDivider, miniDivider);
		BRect topMiddle(frame.left + hDivider, frame.top, frame.right - hDivider, vDivider);
		BRect topRight(frame.right - hDivider, frame.top, frame.right, miniDivider);
		
#ifdef FULL_MODE
		vDivider = miniDivider + thirdScreen;
#endif
		BRect middleLeft(frame.left, miniDivider, frame.left + hDivider, vDivider);
		BRect middleRight(frame.right - hDivider, miniDivider, frame.right, vDivider);

#ifdef FULL_MODE
		BRect leftSide(frame.left, vDivider, frame.left + hDivider, frame.bottom - thirdScreen);
		BRect rightSide(frame.right - hDivider, vDivider, frame.right, frame.bottom - thirdScreen);
	
		vDivider = frame.bottom - thirdScreen;
#endif		
		BRect bottomLeft(frame.left, vDivider, frame.left + hDivider, frame.bottom);
		BRect bottomMiddle(frame.left + hDivider, vDivider, frame.right - hDivider, frame.bottom);
		BRect bottomRight(frame.right - hDivider, vDivider, frame.right, frame.bottom);

		if (where != fPreviousPosition) {
			fPreviousPosition = where;
			ConvertToScreen(&where);

			// use short circuit evaluation for convenience
			if (SwitchModeForRect(where, topLeft, true, true, true, kMiniState)
				|| SwitchModeForRect(where, topMiddle, false, true, true, kExpandoState)
				|| SwitchModeForRect(where, topRight, true, false, true, kMiniState)

				|| SwitchModeForRect(where, middleLeft, true, true, true, kExpandoState)
				|| SwitchModeForRect(where, middleRight, true, false, true, kExpandoState)

#ifdef FULL_MODE
				|| SwitchModeForRect(where, leftSide, true, true, true, kFullState)
				|| SwitchModeForRect(where, rightSide, true, false, true, kFullState)
#endif
				|| SwitchModeForRect(where, bottomLeft, true, true, false, kMiniState)
				|| SwitchModeForRect(where, bottomMiddle, false, true, false, kExpandoState)
				|| SwitchModeForRect(where, bottomRight, true, false, false, kMiniState)
				)
				;
		}

	} else
		BControl::MouseMoved(where, code, message);
}

int32
TDragRegion::DragRegionLocation() const
{
	return fDragLocation;	
}

void
TDragRegion::SetDragRegionLocation(int32 location)
{
	if (location == fDragLocation)
		return;
		
	fDragLocation = location;
	Invalidate();
}
