chessServer: main.o serverStuff.o
	gcc main.o connectionsAcceptor.o errorLogger.o lobbyConnectionsManager.o gameManager.o -std=c17 -pthread -Wall -o chessServer

main.o: main.c
	gcc -c main.c

connectionsAcceptor.o: connectionsAcceptor.c.c
	gcc -c serverStuff.c

errorLogger.o: errorLogger.c
	gcc -c errorLogger.c

lobbyConnectionsManager.o: lobbyConnectionsManager.c
	gcc -c lobbyConnectionsManager.c

gameManager.o: gameManager.c
	gcc -c gameManager.c

clean:
	rm *.o chessServer
