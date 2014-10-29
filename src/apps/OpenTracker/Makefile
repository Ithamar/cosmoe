# makefile for Tracker/Deskbar

SUBDIRS = \
	tracker \
	deskbar

default .DEFAULT :
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f Makefile $@; \
	done
