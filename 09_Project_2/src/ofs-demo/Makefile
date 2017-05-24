GCCFLAGS=-Wall -g
OBJS=ofs-demo.o
EXE=ofs-demo

.PHONY: all clean
all: $(EXE)

$(EXE): $(OBJS)
	gcc $(GCCFLAGS) $(OBJS) -o $(EXE)

ofs-demo.o: ofs-demo.c
	gcc $(GCCFLAGS) -c ofs-demo.c

clean:
	rm -f *.o $(EXE) core*
