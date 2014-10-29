
prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin

CFLAGS  = -pipe -O2 -g `freetype-config --cflags` -c -Wall -Wno-multichar `cat ../../../../cosmoe.specs`
CPPFLAGS= $(CFLAGS) -Woverloaded-virtual -Wnon-virtual-dtor

CC	= gcc
EXE = bmptranslator
COSMOELIBDIR = ../../../../src/kits/objs

OBJS =	BMPMain.o BMPTranslator.o BMPView.o BMPWindow.o

OBJDIR	:= objs

include ../../../../makefile.rules

all :	$(OBJDIR) $(OBJDIR)/$(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/$(EXE): $(OBJS) Makefile
	$(CC) -g -rdynamic $(OBJS) -o $(OBJDIR)/$(EXE) -L$(COSMOELIBDIR) -lcosmoe -lz

install: $(OBJDIR)/$(EXE) $(bindir)
	cp -f $(OBJDIR)/$(EXE) $(bindir)

$(bindir):
	mkdir -p $(bindir)

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE) *~

distclean: clean
	rm -f Makefile

-include $(OBJDIR)/*.d

.PHONY: clean distclean deps doc install uninstall all