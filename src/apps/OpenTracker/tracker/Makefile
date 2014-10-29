# Tracker makefile
# Based on the makefile template

NAME= Tracker
LIB_NAME= libtracker.so

#BIG_TRACKER=TRUE
DEBUGGER= FALSE

ROOT_DIR = ..

#	the build tools are now in the top level directory
BIN_DIR = $(ROOT_DIR)/../bin/

SHARED_INCLUDE_PATHS = shared
LOCAL_INCLUDE_PATHS = $(ROOT_DIR)/$(SHARED_INCLUDE_PATHS)
SYSTEM_INCLUDE_PATHS = .

BERES = $(BIN_DIR)/beres

LIBPATHS = $(ROOT_DIR)/tracker/obj

WARNINGS = \
	-W -Wall \
	-Wno-multichar \
	-Woverloaded-virtual \
	-Wreturn-type \
	-Wnon-virtual-dtor \
	-Wno-ctor-dtor-privacy

ifeq ($(DEBUGGER), TRUE)
	OPTIMIZER := -O0
	DEBUG += -g
else
	OPTIMIZER := -O3
endif
DEPENDENCIES = .x86_dependencies


# @src->@

ORIGIN := ../../../../apps/OpenTracker/

sources_src := \
	AttributeStream.cpp \
	AutoMounter.cpp \
	AutoMounterSettings.cpp \
	BackgroundImage.cpp \
	Bitmaps.cpp \
	ContainerWindow.cpp \
	CountView.cpp \
	DeskWindow.cpp \
	DesktopPoseView.cpp \
	DialogPane.cpp \
	DirMenu.cpp \
	EntryIterator.cpp \
	FBCPadding.cpp \
	FSClipboard.cpp \
	FSUtils.cpp \
	FavoritesConfig.cpp \
	FavoritesMenu.cpp \
	FilePanel.cpp \
	FilePanelPriv.cpp \
	FilePermissionsView.cpp \
	FindPanel.cpp \
	IconCache.cpp \
	IconMenuItem.cpp \
	InfoWindow.cpp \
	MimeTypeList.cpp \
	MiniMenuField.cpp \
	Model.cpp \
	MountMenu.cpp \
	Navigator.cpp \
	NavMenu.cpp \
	NodePreloader.cpp \
	NodeWalker.cpp \
	OpenWithWindow.cpp \
	OverrideAlert.cpp \
	PendingNodeMonitorCache.cpp \
	Pose.cpp \
	PoseList.cpp \
	PoseView.cpp \
	PoseViewScripting.cpp \
	QueryContainerWindow.cpp \
	QueryPoseView.cpp \
	RecentItems.cpp \
	RegExp.cpp \
	SelectionWindow.cpp \
	Settings.cpp \
	SettingsHandler.cpp \
	SettingsViews.cpp \
	SlowContextPopup.cpp \
	SlowMenu.cpp \
	StatusWindow.cpp \
	TaskLoop.cpp \
	TemplatesMenu.cpp \
	Tests.cpp \
	TextWidget.cpp \
	Thread.cpp \
	TitleView.cpp \
	Tracker.cpp \
	TrackerInitialState.cpp \
	TrackerScripting.cpp \
	TrackerSettings.cpp \
	TrackerSettingsWindow.cpp \
	TrackerString.cpp \
	TrashWatcher.cpp \
	Utilities.cpp \
	ViewState.cpp \
	VolumeWindow.cpp \
	WidgetAttributeText.cpp \
#

resources_rdef := \
	TrackerIcons.rdef \
#

resources_app := \
	Tracker.rsrc \
#

# @<-src@ 

SRCS := \
	main.cpp \

ifeq ($(BIG_TRACKER),TRUE)
	SRCS += $(sources_src)
endif

RDEFS := \
	$(resources_rdef) \
#

APP_RSRCS := \
	$(resources_app) \
#

LIB_SRCS := \
	$(sources_src) \
#

# list the libraries your binary needs to link against
# (lbe, ltracker, etc.)
# note that libbe isn't included here because it appears
# that this is included by default on PPC (only).
LIBS= cosmoe translation

#	set the directory where object files and binaries will be created
OBJ_DIR		:= obj

# 	specify that the binary should be created in the object directory
TARGET		:= "$(OBJ_DIR)/$(NAME)"
TARGET_LIB	:= "$(OBJ_DIR)/$(LIB_NAME)"
LIB_EXP		:= $(OBJ_DIR)/$(basename $(LIB_NAME)).exp

MIMESET		:= mimeset
XRES		:= xres

CFLAGS := $(DEBUG) $(OPTIMIZER) $(WARNINGS) \
	-D_BUILDING_tracker=1 -DOPEN_TRACKER=1 \
	-D_INCLUDES_CLASS_DEVICE_MAP=1 \
	-D_SUPPORTS_RESOURCES=1 \
	-D_SUPPORTS_FEATURE_SCRIPTING=1 \
#

LDFLAGS	:= $(DEBUG)

CC	:= gcc
LD	:= gcc

APP_LDFLAGS = $(LDFLAGS) \
			-Xlinker -soname=_APP_

LIB_LDFLAGS = $(LDFLAGS) \
			-Xlinker -soname=$(LIB_NAME) \
			-nostart \
			-e 0 \

BUILD_LIB	= $(LD) -o $@ $(LIB_OBJS) $(LIB_LDFLAGS)

BUILD_EXP = touch "$(LIB_EXP)"


# psuedo-function for converting a list of source files in SRCS variable
# to a corresponding list of object files in $(OBJ_DIR)/xxx.o
# The "function" strips off the src file suffix (.ccp or .c or whatever)
# and then strips of the directory name, leaving just the root file name.
# It then appends the .o suffix and prepends the $(OBJ_DIR)/ path
define SRCS_LIST_TO_OBJS
	$(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(foreach file, $(SRCS), \
	$(basename $(notdir $(file))))))
endef

define LIB_SRCS_LIST_TO_OBJS
	$(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(foreach file, $(LIB_SRCS), \
	$(basename $(notdir $(file))))))
endef

define RDEFS_LIST_TO_RSRCS
	$(addprefix $(OBJ_DIR)/, $(addsuffix .rsrc, $(foreach file, $(RDEFS), \
	$(basename $(notdir $(file))))))
endef

OBJS = $(SRCS_LIST_TO_OBJS)
LIB_OBJS = $(LIB_SRCS_LIST_TO_OBJS)
RSRCS = $(RDEFS_LIST_TO_RSRCS)

# create a unique list of paths to our sourcefiles
SRC_PATHS += $(sort $(foreach file, $(SRCS), $(dir $(file))))

# add source paths to VPATH if not already present
VPATH :=
VPATH += $(addprefix :, $(subst  ,:, $(filter-out $($(subst, :, ,$(VPATH))), $(SRC_PATHS))))

#	SETTING: build the local and system include paths
	LOC_INCLUDES = $(foreach path, $(SRC_PATHS) $(LOCAL_INCLUDE_PATHS), $(addprefix -I, $(path)))
	SYS_INCLUDES = `cat ../../../../cosmoe.specs`
	SYS_INCLUDES += $(foreach path, $(SYSTEM_INCLUDE_PATHS), $(addprefix -I, $(path)))

INCLUDES = $(LOC_INCLUDES) $(SYS_INCLUDES)

# SETTING: add the -L prefix to all library paths to search
LINK_PATHS = $(foreach path, $(SRC_PATHS) $(LIBPATHS), \
	$(addprefix -L, $(path)))

#	SETTING: specify the additional libraries to link against
#	if the libraries have a .so or .a prefix, or if they are _APP_ or _KERNEL_
#	simply add them to the list
LINK_LIBS += $(filter %.so %.a _APP_ _KERNEL_, $(LIBS))
#	if the libraries do not have suffixes and are not _APP_ or _KERNEL_
#	prepend -l to each name: be becomes -lbe
LINK_LIBS += $(foreach lib, $(filter-out %.so %.a _APP_ _KERNEL_, $(LIBS)), $(addprefix -l, $(lib)))

# add to the linker flags 
LDFLAGS += $(LINK_PATHS)  $(LINK_LIBS)

#	SETTING: add the defines to the compiler flags
CFLAGS += $(foreach define, $(DEFINES), $(addprefix -D, $(define)))

#	SETTING: add the additional compiler flags
CFLAGS += $(COMPILER_FLAGS)

#	SETTING: add the additional linker flags
LDFLAGS += $(LINKER_FLAGS)
APP_LDFLAGS += -L$(OBJ_DIR)
ifneq ($(BIG_TRACKER),TRUE)
	APP_LDFLAGS += -ltracker
endif
LIB_LDFLAGS += $(LINKER_FLAGS)

#	define the actual work to be done	
default all: $(TARGET_LIB) $(TARGET)
tracker: $(TARGET)
libtracker: $(TARGET_LIB)

$(TARGET):	$(OBJ_DIR) $(OBJS) $(RSRCS) $(APP_RSRCS)
		$(LD) -o $@ $(OBJS) $(APP_LDFLAGS)
		#$(XRES) -o "$(TARGET)" $(RSRCS) $(APP_RSRCS)
		#$(MIMESET) -f $@

$(TARGET_LIB):	$(OBJ_DIR) $(LIB_OBJS) $(RSRCS) $(LIB_EXP)
		$(BUILD_LIB)
		$(XRES) -o "$(TARGET_LIB)" $(RSRCS)
		$(MIMESET) -f $@
 
$(LIB_EXP):
		$(BUILD_EXP)
 
install:	$(TARGET_LIB) $(TARGET)
		cp $(TARGET_LIB) /system/lib
		copyattr -d $(TARGET) /system


#	rule to create the object file directory if needed
$(OBJ_DIR)::
	@[ -d $(OBJ_DIR) ] || mkdir $(OBJ_DIR) > /dev/null 2>&1

$(OBJ_DIR)/%.o : %.c
	$(CC) -c $< $(INCLUDES) $(CFLAGS) -o $@
$(OBJ_DIR)/%.o : %.cpp
	$(CC) -c $< $(INCLUDES) $(CFLAGS) -o $@
$(OBJ_DIR)/%.rsrc : %.rdef
	$(BERES) -o $@ $<

#	empty rule. Things that depend on this rule will always get triggered
FORCE:

#	The generic clean command. Delete everything in the object folder.
clean :: FORCE
	-rm -rf $(OBJ_DIR)/*

#	remove just the application from the object folder
rmapp ::
	-rm -f "$(TARGET)"

# dependency generation

DEPFLAG = -MM
SEDSCRIPT = sed -e's%^\(.*\.o\)%$$(OBJ_DIR)/\1%'

deps :: FORCE
	echo "# auto-generated by make deps; do not edit" > $(DEPENDENCIES)
	$(CC) $(INCLUDES) $(CFLAGS) $(DEPFLAG) $(sources_src) $(SRCS) | $(SEDSCRIPT) >> $(DEPENDENCIES)

maintainerdeps :: FORCE
	p4 edit $(DEPENDENCIES)
	echo "# auto-generated by make deps; do not edit" > $(DEPENDENCIES)
	$(CC) $(INCLUDES) $(CFLAGS) $(DEPFLAG) $(SRCS) | $(SEDSCRIPT) >> $(DEPENDENCIES)
	echo "Change: new" > /tmp/tmp.deps
	echo "Status: new" >> /tmp/tmp.deps
	echo "Description:" >> /tmp/tmp.deps
	echo "	Auto-generated dependencies" >> /tmp/tmp.deps
	echo "Files:" >> /tmp/tmp.deps
	echo "	//depot/tracker/"$(DEPENDENCIES) >> /tmp/tmp.deps
	cat /tmp/tmp.deps | p4 submit -i

cleandeps :: FORCE
	-rm -f $(DEPENDENCIES)

cleanexp :: FORCE
	rm -f $(LIB_EXP)

-include $(DEPENDENCIES)


