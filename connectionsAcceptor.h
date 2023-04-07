#ifndef CONNECTIONS_ACCEPTOR_H
#define CONNECTIONS_ACCEPTOR_H

#define PORT 54000

//this is the only function exposed in the .h meant to be the start of a
//new pthread. only 1 thread will work with this and 
//subsequent threads will immediately return
void* startAcceptingConnectionsThread(void*);

#endif //CONNECTIONS_ACCEPTOR_H