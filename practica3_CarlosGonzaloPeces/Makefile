CC=gcc
CFLAGS=-g -Wall -O -I. $(shell xeno-config --skin posix --cflags)
LDFLAGS= -L. $(shell xeno-config --skin posix --ldflags)
LDLIBS=-lwiringPi -lrt
all: main
main: main.o fsm.o
clean:
	$(RM) *.o *~ main

libwiringPi.a: wiringPi.o
	ar rcs $@ $^
