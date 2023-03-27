#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <endian.h>

#include "errorLogger.h"

#define INITIAL_CONNECTIONS_CAPACITY 200
#define RECV_MESSAGE_BUFSIZE 1024
#define PAIR_REQUEST_MSGTYPE 7
#define PAIR_REQUEST_MSGSIZE 9

//every function here will be static (not meant to be called from other .c files)
//except for the only function that needs to be exposed in serverStuff.h which is the startServer() function

typedef struct connectionInfo
{
    pthread_t connectionThread;
    int       socket;//the socket used to talk to client (not the listen socket)
    bool      isConnected;
    bool      isPaired;//is the client paired with another opponent or not
    struct    sockaddr_in clientAddr;
    struct    connectionInfo* opponent;
    char      ipPresentation[INET6_ADDRSTRLEN];
}connectionInfo;

//these static variables are not shared between threads so no thread sync (like a mutex) is needed
static connectionInfo* s_connections = NULL;
static size_t          s_numOfConnections = 0;
static size_t          s_connectionsCapactity = INITIAL_CONNECTIONS_CAPACITY;

void startServer()
{
    s_connections = (connectionInfo*)calloc(INITIAL_CONNECTIONS_CAPACITY, sizeof(connectionInfo));
    int listenSocketFd = createListenSocket(NULL, PORT);
    acceptNewConnections(listenSocketFd);
}

//recv a message (blocking) on a socket and return it with the outMessage param. does not handle the return of recv,
//but simply returns it so the callee can handle any error or what to do if the connection was closed.
static int recvMessageIfAvailable(char* outMessage, size_t messageSize, const connectionInfo* clientInfo)
{
    int recvRet = recv(clientInfo->socket, outMessage, messageSize, 0);
    return recvRet;
}

static void closeClient(connectionInfo* client)
{
    client->isPaired = false;
    client->isConnected = false;
    close(client->socket);
    
    //just move the client to the back and lower the number of connections :)
    connectionInfo temp = s_connections[s_numOfConnections - 1];
    s_connections[s_numOfConnections - 1] = *client;
    memcpy(client, &temp, sizeof(connectionInfo));
    --s_numOfConnections;
}

//this is the starting function for a new pthread after accpet() accepts a new client.
//it will wait for the client to make a request to be paired with an opponent
static void* onNewConnection(void* connectionInfoPtr)
{
    connectionInfo* clientInfo = (connectionInfo*)connectionInfoPtr;
    char messageBuffer[RECV_MESSAGE_BUFSIZE] = {};
    clientInfo->isPaired = false;
    clientInfo->isConnected = true;
    
    //initialize the string ip address inside of the connectionInfo structure
    inet_ntop(clientInfo->clientAddr.sin_family,
        &clientInfo->clientAddr.sin_addr, clientInfo->ipPresentation); 
    
    int recvResult = recvMessageIfAvailable(messageBuffer, sizeof(messageBuffer), clientInfo);   
    if(recvResult < 0)
    {
        logError("recv() failed with error", errno);
        closeClient(clientInfo);
        return NULL;
    }
    else if(recvResult == 0)//if the connection was closed
    {
        closeClient(clientInfo);
        return NULL;
    }
    else//we recvieved a message from the client!
    {
        //the first message should be the request to be paired with an opponent...uh oh
        if(recvResult != PAIR_REQUEST_MSGSIZE)
        {
            logError("the first message was not a request to be paired with an opponent");
            closeClient(clientInfo);
            return NULL;
        }
        
        //the message size was correct...
        //but now double check that it has the right message type as its first byte
        if(messageBuffer[0] != PAIR_REQUEST_MSGTYPE)
        {
            logError("the first message was not of type PAIR_REQUEST_MSGTYPE");
            closeClient(clientInfo);
            return NULL;
        }
        
        //ok time to pair the client with a chess opponent...
        
        uint32_t networkByteOrderIP = 0;
        memcpy(&networkByteOrderIP, messageBuffer, sizeof(uint32_t));

        //for now since I dont expect more than 2-10 people to be connected at one time,
        //I am just going to loop over every person connected to search for them.
        //in the future I will make some kind of hash function to have O(1) lookups in a table        
        for(int i = 0; i < s_numOfConnections; ++i)
        {
            if(networkByteOrderIP == s_connections[i].sin_addr.s_addr)
            {
                
            }
        }
        
        clientInfo->isPaired = true;
    }  
    
    return NULL;
}

static void acceptNewConnections(int listenSocketFd)
{   
    //Allocate memory for the connection information then do some initialization in onNewConnection()
    socklen_t sockAddrSize = sizeof(s_connections[0].clientAddr);

    while(true)
    {
        s_connections[s_numOfConnections].socket = accept(listenSocketFd,
            (struct sockaddr*)&(s_connections + s_numOfConnections)->clientAddr, &sockAddrSize); 

        pthread_create(&(s_connections + s_numOfConnections)->connectionThread, 
            NULL, onNewConnection, NULL);

        ++s_numOfConnections;
    }
    
    for(int i = 0; i < s_numOfConnections; ++i)
        pthread_join(s_connections[i].connectionThread, NULL);
    
}

//returns a valid listen socket file discriptor
static int createListenSocket(char const* ip, uint16_t port)
{
    int listenSockfd = -1;
    struct sockaddr_in hints;
    memset(&hints, 0, sizeof(hints));

    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port);
    
    if(ip)
    {
        int ptonRet = inet_pton(AF_INET, ip, &hints.sin_addr);
        if(ptonRet == 0)
        {
            logError("address given to inet_pton is not a valid dotted decimal ip address", 0);
            exit(1);
        }
        else if(ptonRet < 0)
        {
            logError("inet_pton() failed when trying to make the listen socket", 0);
            exit(1);
        }
        
        listenSockfd = socket(PF_INET, SOCK_STREAM, 0);
        if(listenSockfd == -1)
        {
            logError("a call to socket() failed when trying to make the listen socket", 0);     
            exit(1);
        }
    }
    else hints.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(listenSockfd, (struct sockaddr*)&hints, sizeof(hints)) == -1)
    {
        logError("a call to bind() failed when trying to make the listen socket", 0);
        close(listenSockfd);
        exit(1);
    }
      
    if(listen(listenSockfd, SOMAXCONN) == -1)
    {
        logError("a call to listen() failed when trying to make the listen socket", 0);
        close(listenSockfd);
        exit(1);
    }
    
    return listenSockfd;
}