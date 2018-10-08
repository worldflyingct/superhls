CC=gcc
#DEBUG=-DDEBUG
#SAVEFILE=-DSAVEFILE
LIBS=-lmicrohttpd

superhlsserver: main.o datacontroller.o
	$(CC) -o $@ $^ $(LIBS)

main.o: main.c datacontroller.h
	$(CC) -c -o $@ main.c $(DEBUG) $(SAVEFILE)

datacontroller.o: datacontroller.c datacontroller.h
	$(CC) -c -o $@ datacontroller.c $(DEBUG) $(SAVEFILE)

clean:
	rm *.o