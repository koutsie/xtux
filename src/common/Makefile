# Makefile for stuff that's common between server & client.

CC	= gcc
# adding some flags to better society ig:     \/
CFLAGS = -g -Wall -O2 -DDATADIR=\"$(DATADIR)\" -fPIE -fstack-protector-strong -D_FORTIFY_SOURCE=2

AR	= ar
RANLIB	= ranlib

OBJECTS	= common.o timing.o net.o entity_type.o weapon_type.o map.o maths.o datafile.o
XTUXLIB = xtuxlib.a

all:	$(XTUXLIB)

clean:
	rm -f $(OBJECTS) $(XTUXLIB)

$(XTUXLIB):	$(OBJECTS)
	${AR} r $@ $?
	${RANLIB} $@

.c.o:	xtux.h $*.c $*.h
	$(CC) $(CFLAGS) -c $*.c $(COMMON_INC)
