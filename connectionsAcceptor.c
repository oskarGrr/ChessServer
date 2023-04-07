#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <endian.h>

#include "errorLogger.h"
#include "lobbyConnectionsManager.h"

#define RECV_MESSAGE_BUFSIZE 256
#define PORT 54000

static int  createListenSocket(char const* ip, uint16_t port);
static void acceptNewConnections(int listenSock);

//this is the only function exposed in the .h meant to be the start of a new pthread.
//there is only meant to be 1 thread spawned from this
void* startAcceptingConnectionsThread(void*)
{
    //this shouldnt happen but putting it here just to be sure
    static bool oneThreadTest = true;
    if(oneThreadTest) oneThreadTest = false;
    else return NULL;
    
    acceptNewConnections(createListenSocket(NULL, PORT));
}

//If the msgType is a 1 byte type then the msg param is ignored (pass in NULL anyway), and
//only the msgType param is sent. (since it's only a 1 byte message anyway)
//msgSize should be the size of the whole message with the first byte (msgType) included,
//but the msg should only point to the message part without the first msgType byte.
static void buildAndSend(int socket, void* msg, size_t msgSize, uint32_t msgType)
{
    char* buff = calloc(msgSize, sizeof(char));
    
    buff[0] = msgType;//first byte is the msgType (one of the macros above E.g. PAIR_ACCEPT_MSG_SIZE)
    if(msgSize > 1) memcpy(buff + 1, msg, msgSize - 1);//next bytes are the msg if any
    write(socket, buff, msgSize);
    
    free(buff);
}

//error check a call to recv() and return true on success otherwise false
static bool handleRecvReturn(int recvRet, connectionInfo* clientInfo)
{
    if(recvRet > 0) return true;
    
    if(recvRet < 0)
    {
        logError("recv() failed with error", errno);
        closeClient(clientInfo);
    }   
    else if(recvRet == 0)//if the connection was closed
    {
        logError("recv() failed with error", errno);
        closeClient(clientInfo);
    }
    
    return false;
}

//returns the pointer to requested client otherwise NULL
static connectionInfo* getClientByIP(uint32_t nwByteOrderIP)
{
    //for now since I dont expect more than 2-10 people to be connected at one time,
    //I am just going to loop over every person connected to search for them.
    //in the future I will make some kind of hash function to have O(1) lookups in a table        
    for(int i = 0; i < s_numOfConnections; ++i)
    {
        //if the requested IP is connected to the server
        if(nwByteOrderIP == s_connections[i].addrInfo.sin_addr.s_addr)
            return s_connections + i;
    }
    
    return NULL;
}

static void acceptNewConnections(int listenSocketFd)
{   
    socklen_t sockAddrSize = sizeof(struct sockaddr_in);

    bool shouldContinue = true;
    while(shouldConinue)
    {
        struct sockAddr_in addrInfo;
        int socketFd = accept(listenSocketFd, &addrInfo, &sockAddrSize, 0);
        insertLobbyThreadSafe(socketFd, &addrInfo);
    }
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