#!/usr/bin/make -f

SHELL = /bin/sh
CC = gcc
CFLAGS = -O2 -std=c99 -Wall -Wextra -Wshadow -Werror
CPPFLAGS = -I.
LDFLAGS = -s -L.
LDLIBS = -lmqm_r -pthread
OBJS = main.o qmon.o mqcomm.o util.o
PROG = qmon

.SUFFIXES : .c .o

all : $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LDLIBS)

.c.o :
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean :
	-rm $(OBJS)
	-rm *?~
