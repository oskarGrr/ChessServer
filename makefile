chessServer: main.o serverStuff.o
	gcc main.o serverStuff.o errorLogger.o -std=c17 -pthread -Wall -o chessServer

main.o: main.c
	gcc -c main.c

serverStuff.o: serverStuff.c
	gcc -c serverStuff.c

errorLogger.o: errorLogger.c
	gcc -c errorLogger.c

clean:
	rm *.o chessServer
