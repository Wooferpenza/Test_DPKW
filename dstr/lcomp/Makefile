CC:=g++
LD:=ld
AR:=ar
SH:=sh

CFLAGS:= -fPIC -c -O -g -Wall -I$(TOPDIR)/include -L$(TOPDIR)/lib

LIB_VERSION:=1
LIB_PATCHLEVEL:=0
LIB_RELEASE:=
#.$(LIB_VERSION).$(LIB_PATCHLEVEL)

CFLAGS+= -I../include -L../lib -DLCOMP_LINUX=1

LIBS:=liblcomp.so$(LIB_RELEASE)

PARTS:= stubs.o maincomp.o lguids.o ldevbase.o plx.o loadplx.o e440.o loade440.o e140.o loade2010.o e2010.o 791.o e154.o

all: $(LIBS)
 
liblcomp.so$(LIB_RELEASE): $(PARTS)
	$(CC) -shared -Wl,-soname,liblcomp.so -o liblcomp.so$(LIB_RELEASE) $(PARTS) -lc
   

maincomp.o: maincomp.cpp
	$(CC) $(CFLAGS) maincomp.cpp

stubs.o: stubs.cpp
	$(CC) $(CFLAGS) stubs.cpp

lguids.o: lguids.cpp
	$(CC) $(CFLAGS) lguids.cpp

ldevbase.o: ldevbase.cpp
	$(CC) $(CFLAGS) ldevbase.cpp

plx.o: plx.cpp
	$(CC) $(CFLAGS) plx.cpp

loadplx.o: loadplx.cpp
	$(CC) $(CFLAGS) loadplx.cpp
	
e440.o: e440.cpp
	$(CC) $(CFLAGS) e440.cpp

loade440.o: loade440.cpp
	$(CC) $(CFLAGS) loade440.cpp

e140.o: e140.cpp
	$(CC) $(CFLAGS) e140.cpp

loade2010.o: loade2010.cpp
	$(CC) $(CFLAGS) loade2010.cpp

e2010.o: e2010.cpp
	$(CC) $(CFLAGS) e2010.cpp

791.o: 791.cpp
	$(CC) $(CFLAGS) 791.cpp
	
e154.o: e154.cpp
	$(CC) $(CFLAGS) e154.cpp

install:
	cp liblcomp.so /usr/lib/

clean:
	rm *.o
	rm *.so
	