CC=gcc
CFLAGS=-g -Wall -O -I.
LDFLAGS= -L.
LDLIBS=-lwiringPi -lrt
all: main
main: main.o fsm.o
clean:
	$(RM) *.o *~ main

libwiringPi.a: wiringPi.o
	ar rcs $@ $^