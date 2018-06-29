#ifndef PROIECT_SERVER_SERVER_H
#define PROIECT_SERVER_SERVER_H

#define MAX_CLIENTS 1024
#define BUFLEN 256

#include <unordered_map>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include "ClientTerminal.h"
#include "Card.h"

using namespace std;

class Server {
private:
    int portNo;
    ifstream usersData;
    unordered_map<string, Card*> cards;
    unordered_map<int, ClientTerminal*> clients;
    int fdListenTCP;
    int fdUDP;
    struct sockaddr_in serverUDPSockAddr; // ip address + port number
    struct sockaddr_in serverTCPListenSockdAddr;

public:
    Server(int portNo, char* usersDataFile);
    void main();
    void loadData();
};


#endif //PROIECT_SERVER_SERVER_H
