CC=gcc
CFLAGS = -lpthread -O3
DEFS = -DHASHTHREADED
DEPS = csHash.h
OBJ1 = idxserver.o csHash.o
OBJ2 = peer.o csHash.o
OBJ3 = idxserver4.o csHash.o
OBJ4 = peer4.o csHash.o
all: idxserver peer
test: idxserver4 peer4
.PHONY:all
.PHONY:test
.PHONY:clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(DEFS)

idxserver: $(OBJ1)
	$(CC) -o $@ $^ $(CFLAGS)

peer: $(OBJ2)
	$(CC) -o $@ $^ $(CFLAGS)

idxserver4: $(OBJ3)
	$(CC) -o $@ $^ $(CFLAGS)

peer4: $(OBJ4)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.o 
