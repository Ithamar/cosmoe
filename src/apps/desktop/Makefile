prefix=
bindir=${prefix}/bin

CFLAGS   = -g -c -Wall -Wno-multichar -Woverloaded-virtual -Wnon-virtual-dtor `cat ../../cosmoe.specs`
CXXFLAGS = $(CFLAGS)

CC	= gcc
CXX	= g++
EXE	= desktop
COSMOELIBDIR=../../libcosmoe/objs

OBJDIR	:= objs


$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@


OBJS := main.o iconview.o jpeg.o
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))


$(EXE): objs $(OBJS)
	$(CXX) -g $(OBJS) -L/usr/local/lib -lstdc++ -L$(COSMOELIBDIR) -lcosmoe -ljpeg -ldl -lpthread -o $(EXE)

objs:
	mkdir objs

install: $(EXE)
	cp -f $(EXE) $(bindir)
	strip --strip-all $(bindir)/$(EXE)

$(bindir):
	mkdir -p $(bindir)

clean:
	-rm $(EXE) $(OBJS) *~

distclean: clean
	-rm -f Makefile

main.o:		 main.cpp iconview.h
jpeg.o:		 jpeg.cpp
