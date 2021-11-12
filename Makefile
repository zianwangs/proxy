# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -std=c99 -Wno-implicit-function-declaration -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c
socks.o: socks.c
	$(CC) $(CFLAGS) -c socks.c
proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o socks.o csapp.o
	$(CC) $(CFLAGS) proxy.o socks.o csapp.o -o proxy $(LDFLAGS)

clean:
	rm -f *~ *.o proxy 

