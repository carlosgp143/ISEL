CC=gcc
CFLAGS=-g -W -Wall -I$(XENO)/include -I$(XENO)/include/native  -I$(XENO)/include/rtdm -D_GNU_SOURCE -D_REENTRANT
LDFLAGS=-L$(XENO)/lib -Xlinker -rpath $(XENO)/lib -Xlinker $(XENO)/lib/libnative.a $(XENO)/lib/librtdm.a -lpthread -lnative -lrtdm
all: main
main: main.o task.o
clean:
	$(RM) *.o *~ main

