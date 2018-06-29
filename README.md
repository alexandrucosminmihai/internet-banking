# internet-banking

This implementation does not provide any **security**. It is just an exercise project for using TCP and UDP sockets in C++.

## Building
1. Use `make` to build the server and client source files;
2. Use `make clean` to delete the executables.

## Server

The server accepts multiple clients connected at the same time by using multiplexing.

Make sure you start the server first, before trying to connect any clients to it.

### Start up: `./server <server_port> <users_info>`
+ `<server_port>` is a port number you want to use for the server;
+ `<users_info>` is the text file that acts as a data base containing the users' bank accounts information.

### Available **server** commands:
+ `quit` -> shuts down the server

## Client

### Start up: `./client <server_ip> <server_port>`
+ `<server_ip>` -> use `127.0.0.1` if you're running the clients and the server on the same machine;
+ `<server_port>` -> make sure is the same one used to start up the server.

### Available **client** commands:
+ `login <card_number> <pin>`
+ `logout`
+ `listsold` -> works only with authentificated clients and prints the amount of money in the bank account
+ `transfer <destination_cardNo> <sum>`
+ `unlock` -> sends an unlock request for the last card number for which the client attempted to log on
+ `quit` -> shuts down the client
