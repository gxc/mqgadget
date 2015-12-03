#!/usr/bin/make -f
# compile on AIX 6.1 with gcc

SHELL = /bin/sh
CC = gcc
CFLAGS = -O2 -std=c99 -Wall -Wextra -Wshadow -Werror
CPPFLAGS = -DDEBUG -I.
LDFLAGS = -s -L.
LDLIBS = -lmqm_r -pthread
OBJS = qmon.o util.o
PROG = qmon

.SUFFIXES : .c .o

all : $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDLIBS)

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean :
	-rm $(OBJS) $(PROG)
	-rm *?~
