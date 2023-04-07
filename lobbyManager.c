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

//this C file is responsible for the "lobby". the lobby is like a waiting room where
//players are connected to the server, but waiting for a request (or server waiting for them to make request)
//to be paired with another lobby member and play chess, at which point a new thread will
//start to manage the chess game. there is only one single lobby manager thread that sleeps when no one is connected
//and manages all players in the lobby. there might be multiple lobby manager threads in the future if I decide to change it

#define INITIAL_CONNECTIONS_CAPACITY 200

//how long (in seconds) the request to pair up will last before timing out
#define PAIR_REQUEST_TIMEOUT_SECS 10

typedef struct
{
    int    socketFd;
    struct sockaddr_in addr;
    char   ipStr[INET6_ADDRSTRLEN];
}ConnectionInfo;

void connectionInfoCtor(ConnectionInfo* const newConn,
    const int socketFd, const struct sockaddr_in* const addr)
{
    assert(addr);
    newConn->socketFd = socketFd;
    memcpy(&newConn->addr, addr, sizeof(*addr));
    inet_ntop(addr->sin_family, &addr->sin_addr, newConn->ipStr, strlen(newConn->ipStr));
}

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
void insertLobbyThreadSafe(const int socketFd, const struct sockaddr_in* const addr)
{
    assert(addr);
    
    
    
}

//the lobby is like a waiting room where players are connected but not paired and playing chess.
//this func is the start of the lobby manager thread (only 1 thread for now maybe more later)
//which will manage the "lobby" connections.
static void* lobbyManagerThreadStart(void*)
{
    assert(!s_connections);
    s_connections = calloc(s_connectionsCapacity, sizeof(ConnectionInfo));
    
    
}