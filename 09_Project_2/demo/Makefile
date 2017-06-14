GCCFLAGS=-Wall -g
OBJS=demo.o
EXE=demo

.PHONY: all clean
all: $(EXE)

$(EXE): $(OBJS)
	gcc $(GCCFLAGS) $(OBJS) -o $(EXE)

demo.o: demo.c ../openFileSearch.h
	gcc $(GCCFLAGS) -c demo.c

clean:
	rm -f *.o $(EXE) core*
