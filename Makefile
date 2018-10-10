CC=gcc
LIBS=-lmicrohttpd -ljansson

superhlsserver: main.o datacontroller.o config.o memalloc.o
	$(CC) -o $@ $^ $(LIBS)

main.o: main.c datacontroller.h config.h memalloc.h
	$(CC) -c -o $@ main.c

datacontroller.o: datacontroller.c datacontroller.h config.h memalloc.h
	$(CC) -c -o $@ datacontroller.c

config.o: config.c config.h memalloc.h
	$(CC) -c -o $@ config.c

memalloc.o: memalloc.c
	$(CC) -c -o $@ memalloc.c

clean:
	rm *.o