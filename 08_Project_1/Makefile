CC = gcc
CFLAGS = -Wall -pedantic -g
MAIN = seash
MAIN_OBJS = seash.o getcommand.o util.o list.o command.o cd.o arguments.o signal_handling.o execute_commandlist.o pipeline.o

.PHONY: all
all : $(MAIN)

$(MAIN) : $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(MAIN_OBJS)

$(MAIN).o : $(MAIN).c getcommand.h command.h util.h cd.h signal_handling.h execute_commandlist.h pipeline.h
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

arguments.o: arguments.c arguments.h command.h
	$(CC) $(CFLAGS) -c arguments.c

signal_handling.o: signal_handling.c signal_handling.h
	$(CC) $(CFLAGS) -c signal_handling.c

execute_commandlist.o: execute_commandlist.c execute_commandlist.h command.h arguments.h signal_handling.h util.h
	$(CC) $(CFLAGS) -c execute_commandlist.c

pipeline.o: pipeline.c pipeline.h command.h util.h
	$(CC) $(CFLAGS) -c pipeline.c

.PHONY: clean safe
clean :
	rm -f  *.o $(MAIN) core*

safe:
	\cp *.c *.h Makefile ~/.backup
