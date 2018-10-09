CC=gcc
LIBS=-lmicrohttpd -ljansson

superhlsserver: main.o datacontroller.o config.o
	$(CC) -o $@ $^ $(LIBS)

main.o: main.c datacontroller.h config.h
	$(CC) -c -o $@ main.c

datacontroller.o: datacontroller.c datacontroller.h config.h
	$(CC) -c -o $@ datacontroller.c

config.o: config.c config.h
	$(CC) -c -o $@ config.c

clean:
	rm *.o