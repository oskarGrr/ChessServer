#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

//this c file (responsible for one thread) (one thread only here) will 
//loop over all lobby connections and use select() to see if they are 
//trying to make a request to be paired with someone else in the lobby.

#define INITIAL_CONNECTIONS_CAPACITY 200

typedef struct
{
    int    socketFd;
    struct sockaddr_in addrInfo;
    char   ipStr[INET6_ADDRSTRLEN];
}ConnectionInfo;

//insert into the array with the insertNewConnection() function
static connectionInfo* s_connections = NULL;
static size_t s_connectionsCapacity = INITIAL_CONNECTIONS_CAPACITY;
static size_t s_numOfConnections = 0;

static void closeClient(connectionInfo* client)
{
    assert(client);
    close(client->socket);   
    connectionInfo temp = s_connections[s_numOfConnections - 1];
    s_connections[s_numOfConnections - 1] = *client;
    memcpy(client, &temp, sizeof(connectionInfo));
    --s_numOfConnections;
}

//once accept() accepts a new client connection from the thread responsible for
//accepting connections, it will call this func to put them into the "lobby".
void insertNewConnection(connectionInfo* newConn)
{
    assert(newConn);
    
    
}

//start up the connection manager thread (only 1 thread should for this should be made)
//which will manage the "lobby" connections. simply loops over the lobby connections
//and checks if they are trying to be paired with someone else in the lobby.
static void* startConnectionManager(void*)
{
    assert(!s_connections);
    s_connections = calloc(s_connectionsCapacity, sizeof(ConnectionInfo));
    
    
}