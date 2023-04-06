//This is a multithreaded c TCP socket server meant to manage chess connections for my c++ chess game.
//The client automatically connects to this server upon opening the chess client app.
//The client is then put in a loop inside the function onNewConnection() in a new thread.
//from there the server will wait for them to try to make a connection with another player
//that is also connected to the server. After the two clients are paired messages 
//are free to be transfered back and forth between players

#include <stdlib.h>
#include <pthread>
#include "lobbyManager.h"
#include "connectionsAcceptor.h"

int main(void)
{
    pthread_t connectionAcceptorThread;
    pthread_create(&connectionAcceptorThread, NULL, );
    return EXIT_SUCCESS;
}