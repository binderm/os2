CC = gcc
CFLAGS = -Wall -pedantic -g
MAIN = seash
MAIN_OBJS = seash.o getcommand.o util.o list.o command.o

all : $(MAIN)

$(MAIN) : $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(MAIN_OBJS)

$(MAIN).o : $(MAIN).c getcommand.h command.h util.h
	$(CC) $(CFLAGS) -c $(MAIN).c

getcommand.o : getcommand.c getcommand.h command.h util.h list.h
	$(CC) $(CFLAGS) -c getcommand.c

util.o : util.c util.h
	$(CC) $(CFLAGS) -c util.c

list.o : list.c list.h util.h
	$(CC) $(CFLAGS) -c list.c

command.o : command.c command.h list.h util.h
	$(CC) $(CFLAGS) -c command.c

clean :
	rm *.o $(MAIN) core*

safe:
	\cp *.c *.h Makefile ~/.backup
