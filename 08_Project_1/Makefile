CC = gcc
CFLAGS = -Wall -pedantic -g
MAIN = seash
MAIN_OBJS = seash.o getcommand.o util.o list.o command.o cd.o execute_commandlist.o

.PHONY: all
all : $(MAIN)

$(MAIN) : $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(MAIN_OBJS)

$(MAIN).o : $(MAIN).c getcommand.h command.h util.h cd.h execute_commandlist.h
	$(CC) $(CFLAGS) -c $(MAIN).c

getcommand.o : getcommand.c getcommand.h command.h util.h list.h
	$(CC) $(CFLAGS) -c getcommand.c

util.o : util.c util.h
	$(CC) $(CFLAGS) -c util.c

list.o : list.c list.h util.h
	$(CC) $(CFLAGS) -c list.c

command.o : command.c command.h list.h util.h
	$(CC) $(CFLAGS) -c command.c

cd.o : cd.c cd.h command.h
	$(CC) $(CFLAGS) -c cd.c

execute_commandlist.o: execute_commandlist.c execute_commandlist.h command.h
	$(CC) $(CFLAGS) -c execute_commandlist.c

.PHONY: clean safe
clean :
	rm -f  *.o $(MAIN) core*

safe:
	\cp *.c *.h Makefile ~/.backup
