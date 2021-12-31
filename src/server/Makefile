#======================================================#
# Makefile for XTux Arena SERVER                       #
# (c) 1999-2000 David Lawrence - philaw@ozemail.com.au #
#======================================================#

#uncomment for Solaris (what is the OSTYPE variable?)
#SOCKLIB = -lsocket -lnsl

# PATHS
TOPLEVEL_PATH = ../..
COMMON_PATH = $(TOPLEVEL_PATH)/src/common

# Programs used in building process
CC = gcc

# Where to put the XTux client binary.
BINARY = $(TOPLEVEL_PATH)/tux_serv

# Flags used when compiling
# adding some flags to better society ig:     \/
CFLAGS = -g -Wall -O2 -DDATADIR=\"$(DATADIR)\" -fPIE -fstack-protector-strong -D_FORTIFY_SOURCE=2


# Paths to header files
COMMON_INC = -I$(COMMON_PATH)

# Libraries used in linking
MATHLIB = -lm

# Common libraries
XTUXLIB = $(COMMON_PATH)/xtuxlib.a

# Objects to build
OBJECTS = main.o sv_net.o sv_netmsg_recv.o sv_netmsg_send.o world.o game.o entity.o weapon.o misc.o sv_map.o ai.o hitscan.o item.o event.o

all:	server

clean:
	rm -f $(OBJECTS) $(BINARY)

#Apps
server:	$(OBJECTS)
	$(CC) -o $(BINARY) $(OBJECTS) $(XTUXLIB) $(MATHLIB) $(SOCKLIB) $(GGZLIB)

# OBJECTS
.c.o:	server.h $*.h $*.c
	$(CC) $(CFLAGS) -c $*.c $(COMMON_INC) $(GGZ_INC)
