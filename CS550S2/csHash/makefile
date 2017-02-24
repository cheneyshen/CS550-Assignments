
#CC=gcc-4.9
CC=gcc
CFLAGS = -lpthread -O3
DEFS = -DHASHTHREADED
//DEFS = -DHASHTEST -DHASHTHREADED
DEPS = csHash.h
OBJ = prac2.o csHash.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY:
	clean

clean:
	rm *.o test
