CC=gcc
CFLAGS=-O3
LIBS=-lmicrohttpd -ljansson

superhlsserver: main.o datacontroller.o config.o memalloc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

main.o: main.c datacontroller.h config.h memalloc.h
	$(CC) $(CFLAGS) -c -o $@ main.c

datacontroller.o: datacontroller.c datacontroller.h config.h memalloc.h
	$(CC) $(CFLAGS) -c -o $@ datacontroller.c

config.o: config.c config.h memalloc.h
	$(CC) $(CFLAGS) -c -o $@ config.c

memalloc.o: memalloc.c
	$(CC) $(CFLAGS) -c -o $@ memalloc.c

clean:
	rm *.o