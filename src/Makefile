CC:=g++
LD:=ld
AR:=ar
SH:=sh

CFLAGS:= -D_REENTERANT -DLCOMP_LINUX=1 -rdynamic -O2 -Wall -I$(TOPDIR)/include -L$(TOPDIR)/lib


CFLAGS+= -I../include  -L/usr/lib/nptl -ldl -lpthread

   
test: LCard.cpp
	$(CC) LCard.cpp $(CFLAGS) -o LCard 

clean:
	rm LCard
