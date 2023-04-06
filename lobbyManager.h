#ifndef LOBBY_MANAGER_H
#define LOBBY_MANAGER_H

//insert a new connected client into the "lobby".
bool insertNewLobbyConnection(const int socketFd, const struct sockaddr_in* const addr);

#endif //LOBBY_MANAGER_H