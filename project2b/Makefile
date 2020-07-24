CFLAGS=-g -Wall
CC=clang
SRCS=tokenizer.c penn-sh.c
OBJS=tokenizer.o penn-sh.o
LDFLAGS=
LIBS=

all:    penn-sh

$(SRCS):
	$(CC) $(CFLAGS) -c $*.c

penn-sh: $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o penn-sh $(OBJS)

clean:
	rm -f *.o penn-sh
