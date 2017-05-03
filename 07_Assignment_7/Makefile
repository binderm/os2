GCC_FLAGS=-Wall -g
OBJS_SIGNAL_FUN=signalFun.o
OBJS_C1=c1.o
OBJS_C2=c2.o
EXE_SIGNAL_FUN=signalFun
EXE_C1=c1
EXE_C2=c2

.PHONY: all
all: $(EXE_SIGNAL_FUN) $(EXE_C1) $(EXE_C2)

$(EXE_SIGNAL_FUN): $(OBJS_SIGNAL_FUN)
	gcc $(GCC_FLAGS) $(OBJS_SIGNAL_FUN) -o $(EXE_SIGNAL_FUN)
$(EXE_C1): $(OBJS_C1)
	gcc $(GCC_FLAGS) $(OBJS_C1) -o $(EXE_C1)
$(EXE_C2): $(OBJS_C2)
	gcc $(GCC_FLAGS) $(OBJS_C2) -o $(EXE_C2)

signalFun.o: signalFun.c
	gcc $(GCC_FLAGS) -c signalFun.c
c1.o: c1.c
	gcc $(GCC_FLAGS) -c c1.c
c2.o: c2.c
	gcc $(GCC_FLAGS) -c c2.c

.PHONY: clean
clean:
	rm -f *.o $(EXE_SIGNAL_FUN) $(EXE_C1) $(EXE_C2)
