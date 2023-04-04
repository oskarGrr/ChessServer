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

#define INITIAL_CONNECTIONS_CAPACITY 200
#define RECV_MESSAGE_BUFSIZE 256
#define PORT 54000

//all of the below macros will be present in the client source as well

#define WHICH_SIDE_MSGTYPE 6
#define WHICH_SIDE_MSG_SIZE 2

#define PAIR_REQUEST_MSGTYPE 7
#define PAIR_REQUEST_MSG_SIZE 5

#define PAIR_ACCEPT_MSGTYPE 8
#define PAIR_ACCEPT_MSG_SIZE 5

#define PAIR_DECLINE_MSGTYPE 9
#define PAIR_DECLINE_MSG_SIZE 1

//when the requested opponent is already playing against someone else
#define ALREADY_PAIRED_MSGTYPE 10
#define ALREADY_PAIRED_MSG_SIZE 1

//when the opponent doesnt respond in less than
//PAIR_REQUEST_TIMEOUT_SECS seconds to a request to play chess
#define PAIR_NORESPONSE_MSGTYPE 11
#define PAIR_NORESPONSE_MSG_SIZE 1

//how long (in seconds) the request to pair up will last before timing out
#define PAIR_REQUEST_TIMEOUT_SECS 10

//every function here will be static (not meant to be called from other .c files)
//except for the only function that needs to be exposed in serverStuff.h which is the startServer() function

static int createListenSocket(char const*, uint16_t);
static void acceptNewConnections(int);

//stores info about the client and the chess game
typedef struct connectionInfo
{
    pthread_t connectionThread;
    int       socket;//the socket used to talk to client (not the listen socket)
    bool      isConnected;
    bool      isPaired;//is the client paired with another opponent or not
    enum      {INVALID, BLACK, WHITE} side;
    struct    sockaddr_in addrInfo;
    struct    connectionInfo* opponent;
    struct    connectionInfo* potentialOpponent;//used durring the pairing proccess
    char      ipPresentation[INET6_ADDRSTRLEN];
}connectionInfo;

static connectionInfo* s_connections = NULL;
static size_t          s_numOfConnections = 0;
static size_t          s_connectionsCapactity = INITIAL_CONNECTIONS_CAPACITY;

void startServer(void)
{
    srand(time(NULL));
    s_connections = (connectionInfo*)calloc(INITIAL_CONNECTIONS_CAPACITY, sizeof(connectionInfo));
    int listenSocketFd = createListenSocket(NULL, PORT);
    acceptNewConnections(listenSocketFd);
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

//the first part of the pairing proccess will wait for the client to make a request to be paired.
//once the request is made, a PAIR_REQUEST_MSGTYPE is sent to the potential opponent with the clients IP.
static void waitForPairingStage1(connectionInfo* clientInfo)
{
    char messageBuffer[RECV_MESSAGE_BUFSIZE] = {};
    int recvResult = recv(clientInfo->socket, messageBuffer, sizeof(messageBuffer), 0);
    if(!handleRecvReturn(recvResult, clientInfo)) return;

    if(recvResult != PAIR_REQUEST_MSG_SIZE || messageBuffer[0] != PAIR_REQUEST_MSGTYPE)
    {
        logError("the first message was not a request to be paired with an opponent", 0);
        closeClient(clientInfo);
        return;
    }

    uint32_t nwByteOrderOpponentIP = 0;
    memcpy(&nwByteOrderOpponentIP, messageBuffer, sizeof(uint32_t));
    connectionInfo* potentialOpponent = getClientByIP(nwByteOrderOpponentIP);
    if(potentialOpponent)//if they are connected to the server
    {
        //if they are already playing against someone else
        if(potentialOpponent->isPaired)
        {
            buildAndSend(clientInfo->socket, NULL,
                ALREADY_PAIRED_MSGTYPE, ALREADY_PAIRED_MSG_SIZE);
        }
    
        //send a pair request along with the clients ip to the potential opponent        
        buildAndSend(clientInfo->socket, &clientInfo->addrInfo.sin_addr, 
            PAIR_REQUEST_MSG_SIZE, PAIR_REQUEST_MSGTYPE);
            
        clientInfo->potentialOpponent = potentialOpponent;
    }
}

//stage 2 of the pairing procces will wait for the response from the potential opponent.
//if the potential opponent responds with yes, then pick sides and the pairing proccess is complete.
static void waitForPairingStage2(connectionInfo* client)
{
    fd_set socketSet;
    FD_ZERO(&socketSet);
    FD_SET(client->potentialOpponent->socket, &socketSet);
    struct timeval timeValue = {.tv_sec = 10};
    
    //check if a call to recv will block
    int selectRet = select(client->potentialOpponent->socket + 1,
        &socketSet, NULL, NULL, &timeValue);
    if(selectRet == -1)//select failed
    {
        logError("select failed with error", errno);
        return;
    }
    else if(selectRet == 0)//select expired
    {
        buildAndSend(client->socket, NULL,
            PAIR_NORESPONSE_MSGTYPE, PAIR_NORESPONSE_MSG_SIZE);        
        return;
    }

    char recvBuff[RECV_MESSAGE_BUFSIZE] = {};
    int recvRet = recv(client->potentialOpponent->socket, recvBuff, sizeof(recvBuff), 0);
    if(!handleRecvReturn(recvRet, client->potentialOpponent)) return;

    //wrong message type
    if(recvRet != PAIR_ACCEPT_MSG_SIZE || recvBuff[0] != PAIR_ACCEPT_MSGTYPE)
    {
        closeClient(client->potentialOpponent);
        return;
    }

    buildAndSend(client->socket, &client->potentialOpponent->addrInfo.sin_addr, 
        PAIR_ACCEPT_MSG_SIZE, PAIR_ACCEPT_MSGTYPE);  
    
    uint8_t oppositeSide = 0;
    if(rand() & 1)
    {
        client->side = WHITE;
        oppositeSide = BLACK;
    }
    else
    {
        client->side = BLACK;
        oppositeSide = WHITE;
    }
    
    buildAndSend(client->socket, &client->side, 
        WHICH_SIDE_MSG_SIZE, WHICH_SIDE_MSGTYPE);
    
    buildAndSend(client->potentialOpponent->socket, &oppositeSide, 
        WHICH_SIDE_MSG_SIZE, WHICH_SIDE_MSGTYPE);
    
    client->isPaired = true;
    client->opponent = client->potentialOpponent;
    client->potentialOpponent = NULL;
}

//Called in onNewConnection() once the server is ready to 
//listen for a request to be paired with an opponent.
static bool waitForPairing(connectionInfo* clientInfo)
{
    while(!clientInfo->isPaired)
    {
        waitForPairingStage1(clientInfo);
        if(!clientInfo->isConnected) return false;
        waitForPairingStage2(clientInfo);
        if(!clientInfo->isConnected) return false;
    }
    
    return true;
}

//called after waitForPairing() when the players are ready to play
static void mainChessMessageLoop(connectionInfo* clientInfo)
{
    
}

//This is the starting function for a new pthread after accpet() accepts a new client.
//It will wait for the client to make a request to be paired with an opponent.
static void* onNewConnection(void* connectionInfoPtr)
{
    connectionInfo* clientInfo = (connectionInfo*)connectionInfoPtr;
    clientInfo->isConnected = true;
    clientInfo->isPaired = false;
    clientInfo->side = INVALID;
    
    //initialize the string ip address inside of the connectionInfo structure
    inet_ntop(clientInfo->addrInfo.sin_family, 
        &clientInfo->addrInfo.sin_addr, clientInfo->ipPresentation, INET6_ADDRSTRLEN); 
    
    //wait for the user to request to play with someone
    if(!waitForPairing(clientInfo)) return NULL;   
    
    //todo start sending messages back and forth
    
    //End of this thread. Everything should be handled with the client and the connection should be closed here.
    return NULL;
}

static void acceptNewConnections(int listenSocketFd)
{   
    socklen_t sockAddrSize = sizeof(s_connections[0].addrInfo);

    while(s_numOfConnections < s_connectionsCapactity)
    {
        s_connections[s_numOfConnections].socket = accept(listenSocketFd,
            (struct sockaddr*)&(s_connections + s_numOfConnections)->addrInfo, &sockAddrSize); 

        //INIT CONNECTION INFO HERE
        
        pthread_create(&(s_connections + s_numOfConnections)->connectionThread, 
            NULL, onNewConnection, NULL);//FORGOT TO PASS CONNECTION INFO

        ++s_numOfConnections;
    }
    
    for(int i = 0; i < s_connectionsCapactity; ++i)
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