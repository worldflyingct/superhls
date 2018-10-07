CC=gcc
LIBS=-lmicrohttpd

superhlsserver: main.o datacontroller.o
	$(CC) -o $@ $^ $(LIBS)

main.o: main.c datacontroller.h
	$(CC) -c -o $@ main.c

datacontroller.o: datacontroller.c datacontroller.h
	$(CC) -c -o $@ datacontroller.c

clean:
	rm *.o