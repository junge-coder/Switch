LIBS=$(LIB) -lpthread
OBJ = switch.o  dcs_log.o socketfuc.o base64.o pubfuc.o
BIN =$(HOME)/run/bin
.c.o:
	$(CC) -c $*.c 

EXE = switch 
all: $(OBJ) $(EXE) cleanup

switch: $(OBJ)
	$(CC) -o switch $(OBJ) $(LIBS)
	strip switch 2>/dev/null
#	mv switch $(BIN)/switch

cleanup:
	rm -f *.o
