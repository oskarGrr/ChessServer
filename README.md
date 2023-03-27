WORK IN PROGRESS

This is a multithreaded c TCP socket server meant to manage chess connections for my c++ chess game. 
The client automatically connects to this server upon opening the chess client. 
The client is then put in a loop inside the function onNewConnection() in a new thread. 
From there the server will wait for them to try to make a connection with another player 
that is also connected to the server. After the two clients agree to the challenge and are paired,
they are then are free to communicate with each other.