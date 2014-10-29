prefix=
bindir=${prefix}/bin

CFLAGS   = -g -c -Wall -Wno-multichar -Woverloaded-virtual -Wnon-virtual-dtor `cat ../../cosmoe.specs`

OBJS	= main.o tview.o
EXE	= cterm
COSMOELIBDIR = ../../libcosmoe/objs

CXX	= g++
LD	= g++

all: $(EXE)

$(EXE): $(OBJS)
	$(LD) $(OBJS) -L$(COSMOELIBDIR) -lcosmoe -ldl -lpthread -o $@

install: $(EXE) $(bindir)
	cp -f $(EXE) $(bindir)

$(bindir):
	mkdir -p $(bindir)

clean:
	-rm -f $(EXE) $(OBJS) *~

distclean: clean
	-rm -f Makefile

.cpp.o:
	$(CXX) $(CFLAGS) $< -o $@

main.o:		main.cpp tview.h
tview.o:	tview.cpp tview.h
