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
#include "Server.h"

Server::Server(int portNo, char* usersDataFile) {
    int rc = 0;
    this->portNo = portNo;

    // opening the file with the users database
    this->usersData.open(usersDataFile);
    if (!this->usersData) {
        cerr << "Error: User Data File could not be open\n";
        exit(-10);
    }

    // open the UDP socket
    fdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (fdUDP < 0) {
        cerr << "Error: the server could not open its UDP socket\n";
        exit(-10);
    }
    serverUDPSockAddr.sin_family = AF_INET; // we are using IPv4
    serverUDPSockAddr.sin_addr.s_addr = htonl(INADDR_ANY); // listen for connections coming from any address
    serverUDPSockAddr.sin_port = htons((unsigned short)portNo); // the port that this socket is binded to
    rc = bind(fdUDP, (struct sockaddr *) &serverUDPSockAddr, sizeof(serverUDPSockAddr));
    if (rc < 0) {
        cerr << "Error: server could not bind the UDP socket\n";
        exit(-10);
    }

    // open the TCP listen socket
    fdListenTCP = socket(AF_INET, SOCK_STREAM, 0);
    if (fdListenTCP < 0) {
        cerr << "Error: the server could not open its TCP listen socket\n";
        exit(-10);
    }
    serverTCPListenSockdAddr = serverUDPSockAddr; // both sockets use the same address (IP + port)
    rc = bind(fdListenTCP, (struct sockaddr *) &serverTCPListenSockdAddr, sizeof(serverTCPListenSockdAddr));
    if (rc < 0) {
        cerr << "Error: server could not bind the TCP listen socket\n";
        exit(-10);
    }

    // set the socket to be reusable as soon as the currently using process is shut down
    int enable = 1;
    rc = setsockopt(fdListenTCP, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rc < 0) {
        cerr << "Error: unable to set reusability option for the server's TCP socket!\n";
        exit(-10);
    }

    listen(fdListenTCP, MAX_CLIENTS); // mark this socket as a listening socket
}

void Server::loadData() {
	/*
		Read the file given as argument which contains information about clients' cards,
		create a Card object for each entry in the file and hold the objects in the cards map
		that links a string (a card number) to a Card* .
	*/
    int i;
    int N;
    string lastName, firstName, cardNo, pin, password, sumString;
    double sum;

    usersData >> N;
    for (i = 0; i < N; ++i) {
        usersData >> lastName;
        usersData >> firstName;
        usersData >> cardNo;
        usersData >> pin;
        usersData >> password;
        usersData >> sumString;

        sum = stod(sumString, nullptr);

        cards[cardNo] = new Card(lastName, firstName, cardNo, pin, password, sum);
    }

    usersData.close();
}

void Server::main() {
    int rc;
    char buffer[BUFLEN];

    loadData(); // load the data from the parameter file
    /*
		The UDP socket and the TCP listen socket were created by the constructor.
		Up next, I will multiplex the following file descriptors:
		-> the UDP socket
		-> the TCP listen socket
		-> the TCP sockets used for communication with the clients
		-> stdin for receiving the quit command inside the server
    */
    fd_set read_fds;
    fd_set tmp_fds;
    int fdMax = fdListenTCP;
    if (fdUDP > fdMax) {
        fdMax = fdUDP;
    }

    // I make sure that the reading sockets set and the auxilary sockets set are empty
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    FD_SET(fdListenTCP, &read_fds); // add the TCP listen socket
    FD_SET(fdUDP, &read_fds); // add the UDP socket
    FD_SET(0, &read_fds); // add STDIN

    int ok = 1;
    while (ok) { // main loop
        tmp_fds = read_fds;
        // keep only the active sockets inside the tmp_fds auxilary set
        rc = select(fdMax + 1, &tmp_fds, NULL, NULL, NULL); 
        if (rc == -1) {
            cerr << "Error: function call to select(...) in the server's main loop\n";
            exit(-10);
        }

        for (int i = 0; i <= fdMax; ++i) {
            if (!FD_ISSET(i, &tmp_fds)) { // nothing was sent to this socket
                continue;
            }

            if (i == 0) {
                // the server received input from stdin
                memset(buffer, 0, BUFLEN);
                fgets(buffer, BUFLEN - 1, stdin);
                buffer[strlen(buffer) - 1] = '\0';

                if (strcmp(buffer, "quit") != 0) {
                    cerr << "Error: the server can only accept the \'quit\' command which shuts it down";
                    continue;
                }

                memset(buffer, 0, BUFLEN);
                sprintf(buffer, "IBANK> The server is shutting down!");

                // send the quit message to all clients connected over TCP to the server
                for (int j = 1; j <= fdMax; ++j) {
                    if (FD_ISSET(j, &read_fds)) {
                        if (j != fdListenTCP && j != fdUDP) {
                            send(j, buffer, strlen(buffer), 0);
                        }
                        close(j);
                    }
                }

                ok = 0;

                break;
            }

            if (i == fdUDP) {
                // the server received an unlock request or a client's secret password
                struct sockaddr_in clientAddr;
                socklen_t clen = sizeof(clientAddr);

                rc = recvfrom(fdUDP, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, &clen);
                if (rc < 0) {
                    cerr << "Error: server unable to receive message via the UDP socket\n";
                    exit(-10);
                }

                char *p = strtok(buffer, " ");
                if (strcmp(p, "unlock") == 0) {
                    // if an unlock request is made now
                    p = strtok(NULL, " ");
                    string toUnlockCardNo = string(p);
                    memset(buffer, 0, BUFLEN);

                    if (cards.find(toUnlockCardNo) == cards.end()) {
                        // if the card number does not exist in memory
                        sprintf(buffer, "UNLOCK> -4 : Unexistent card number");
                    } else {
                        Card* currCard = cards[toUnlockCardNo];

                        if (currCard->getStatus() == 2) { // CARD_LOCKED = 2
                            // if the card for which the request was made is truly blocked
                            currCard->setStatus(3); // CARD_UNLOCKREQUESTED = 3
                            sprintf(buffer, "UNLOCK> Send the secret password");
                        } else if (currCard->getStatus() == 3) {
                            sprintf(buffer, "UNLOCK> -7 : Unlocking failed");
                        } else {
                            // if the card exists but it isn't blocked
                            sprintf(buffer, "UNLOCK> -6 : Failed operation");
                        }
                    }

                } else {
                    // if the unlock request was already made, the secret password must be received
                    string toUnlockCardNo = string(p);
                    p = strtok(NULL, " ");
                    string secretPassword = string(p);
                    memset(buffer, 0, BUFLEN);

                    if (cards.find(toUnlockCardNo) == cards.end()) {
                        // if the card number does not exist in memory
                        memset(buffer, 0, BUFLEN);
                        sprintf(buffer, "UNLOCK> -4 : Unexistent card number");
                    } else {
                        Card *currCard = cards[toUnlockCardNo];

                        if (currCard->getStatus() == 3) { // CARD_UNLOCKREQUESTED = 3
                            // if there was an unlock request made for this card
                            if (currCard->getPassword() == secretPassword) {
                                // if the password is correct
                                currCard->setStatus(0); // CARD_OFFLINE = 0
                                sprintf(buffer, "UNLOCK> The card was unlocked");
                            } else {
                                // if the password is wrong, reset the card status to LOCKED
                                currCard->setStatus(2); // CARD_LOCKED = 2
                                sprintf(buffer, "UNLOCK> -7 : Unlocking failed");
                            }
                        } else {
                            // there was no request to unlock this card
                            sprintf(buffer, "UNLOCK> -7 : Unlocking failed");
                        }
                    }
                }

                rc = sendto(fdUDP, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, clen);
                if (rc < 0) {
                    cerr << "Error: server was unable to send message over UDP\n";
                    exit(-10);
                }

                continue;
            }

            if (i == fdListenTCP) {
                cerr << "Status update: a client is trying to connect to the server over TCP\n";
                // the server received a request from a client to open a TCP socket
                struct sockaddr_in clientAddr;
                socklen_t clen = sizeof(clientAddr);
                int newSocketFD;

                newSocketFD = accept(fdListenTCP, (struct sockaddr *) &clientAddr, &clen);
                if (newSocketFD == -1) {
                    cerr << "Error: server unable to open new TCP socket for communicating with a new client\n";
                    exit(-10);
                }

                FD_SET(newSocketFD, &read_fds);
                if (newSocketFD > fdMax) {
                    fdMax = newSocketFD;
                }

                continue;
            }

            // the server received a command from an already connected client
            memset(buffer, 0, BUFLEN);
            int n = recv(i, buffer, sizeof(buffer), 0);
            if (n < 0) {
                cerr << "Error: server unable to receive message from a client over TCP\n";
                exit(-10);
            }

            if (n == 0) {
                // for some reason the client shut down without sending a quit command
                close(i);
                FD_CLR(i, &read_fds);
                continue;
            }

            buffer[strlen(buffer) - 1] = '\0';

            // check the received command and execute it
            char *p = strtok(buffer, " ");

            /*
				before checking the received command, the server checks wheter it must receive a confirmation
				for a transfer and if so, the server checks if the input begins with y (for yes) or not
            */
            if (clients.find(i) != clients.end()) {
                ClientTerminal* currClient = clients[i]; // pointer to the ClientTerminal connected on socket i
                if (currClient->getStatus() == 2) { // CLIENT_PENDINGTRANSFERAPPROVAL = 2
                    // if the server was waiting for a confirmation from this client
                    if (p[0] == 'y') {
                        // if the server received the confirmation, then complete the transfer
                        Card* currCard;
                        Card* destCard;
                        double transferSum = 0;
                        currCard = cards[currClient->getCurrCardNo()];
                        destCard = cards[currClient->getDestCardNo()];

                        transferSum = currClient->getPendingSum();
                        currCard->setSum(currCard->getSum() - transferSum);
                        destCard->setSum(destCard->getSum() + transferSum);

                        memset(buffer, 0, BUFLEN);
                        sprintf(buffer, "IBANK> Transfer completed successfully");
                        send(i, buffer, sizeof(buffer), 0);
                    } else {
                        memset(buffer, 0, BUFLEN);
                        sprintf(buffer, "IBANK> -9 : Operation was canceled");
                        send(i, buffer, sizeof(buffer), 0);
                    }

                    // reset the state of the client
                    currClient->setStatus(1); // CLIENT_CONNECTED = 1
                    currClient->setDestCardNo("");
                    currClient->setPendingSum(0);

                    continue;
                }
            }

            // I begin to check the effective commands

            // --- login <card_no> <pin>
            if (strcmp(p, "login") == 0) {
                p = strtok(NULL, " ");
                string cardNo = string(p);
                p = strtok(NULL, " ");
                string pin = string(p);

                if (clients.find(i) == clients.end()) {
                    // if this client has never connected to the server on no other card in the current session, then I mark him in the map
                    ClientTerminal* newClient = new ClientTerminal(cardNo, 0, i, 0);
                    clients[i] = newClient;
                }


                ClientTerminal* currClient = clients[i];
                if (currClient->getCurrCardNo() != cardNo) {
                    // if the client tried another card before
                    currClient->setCurrCardNo(cardNo);
                    currClient->setPinFailNo(0);
                }

                if (cards.find(cardNo) == cards.end()) {
                    // if this card number does not exist in memory
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -4 : Unexistent card number");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                Card* card = cards[cardNo];
                if (card->getStatus() == 1) { // CARD_ONLINE = 1
                    // if there is another active session for this card
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -2 : Session already active");
                    send(i, buffer, sizeof(buffer), 0);
                    currClient->setPinFailNo(0); // reset the streak of consecutive failed pin attempts
                    continue;
                }

                if (card->getStatus() != 0) { // CARD_LOCKED = 2
                    // if this card is blocked
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -5 : Card blocat");
                    send(i, buffer, sizeof(buffer), 0);
                    currClient->setPinFailNo(0); // reset the streak of consecutive failed pin attempts
                    continue;
                }


                if (card->getPin() != pin) {
                	// if the input pin is incorrect
                    currClient->setPinFailNo(currClient->getPinFailNo() + 1);
                    if (currClient->getPinFailNo() == 3) {
                        card->setStatus(2); // CARD_LOCKED = 2
                        currClient->setPinFailNo(0);
                        memset(buffer, 0, BUFLEN);
                        sprintf(buffer, "IBANK> -5 : Card locked");
                        send(i, buffer, sizeof(buffer), 0);
                    } else {
                        memset(buffer, 0, BUFLEN);
                        sprintf(buffer, "IBANK> -3 : Incorrect pin");
                        send(i, buffer, sizeof(buffer), 0);
                    }

                    continue;
                }

                // if we reached this spot, the pin is correct
                currClient->setPinFailNo(0);
                // mark the fact that the user is logged on a card in the current session
                currClient->setStatus(1); // CLIENT_CONNECTED = 1
                card->setStatus(1); // CARD_ONLINE = 1

                memset(buffer, 0, BUFLEN);
                sprintf(buffer, "IBANK> Welcome %s %s", card->getLastName().c_str(), card->getFirstName().c_str());
                send(i, buffer, sizeof(buffer), 0);

                continue;
            }

            // --- logout
            if (strcmp(p, "logout") == 0) {

                if (clients.find(i) == clients.end()) {
                    // if the user didn't even try to log on a card in the current session
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -1 : The client is not authentified");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                ClientTerminal* currClient = clients[i]; // pointer to the ClientTerminal connected on socket i
                if (currClient->getStatus() != 1) { // CLIENT_CONNECTED = 1
                    // if the user was not successfully logged on any card
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -1 : The client is not authentified");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                string cardNo = clients[i]->getCurrCardNo();
                if (cards.find(cardNo) != cards.end()) {
                    cards[cardNo]->setStatus(0);
                }
                clients.erase(i); // remove the client from the clients map. When he/she will log in again, they will be readded
                memset(buffer, 0, BUFLEN);
                sprintf(buffer, "IBANK> The client was disconnected");
                send(i, buffer, sizeof(buffer), 0);

                continue;
            }

            // --- listsold (list the amount of money in the account)
            if (strcmp(p, "listsold") == 0) {
                
                if (clients.find(i) == clients.end()) {
                    // if the user didn't even try to log on a card in the current session 
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -1 : The client is not authentified");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                ClientTerminal* currClient = clients[i]; // pointer to the ClientTerminal connected on socket i
                if (currClient->getStatus() != 1) { // CLIENT_CONNECTED = 1
                    // if the user was not successfully logged on any card
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -1 : The client is not authentified");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                string cardNo = currClient->getCurrCardNo();
                double sum = cards[cardNo]->getSum();
                memset(buffer, 0, BUFLEN);
                sprintf(buffer, "IBANK> %.2lf", sum);
                send(i, buffer, sizeof(buffer), 0);

                continue;
            }

            // --- transfer <card_no> <sum>
            if (strcmp(p, "transfer") == 0) {
                p = strtok(NULL, " ");
                string destCardNo = string(p);
                p = strtok(NULL, " ");
                double transferSum = strtod(p, NULL);

                if (clients.find(i) == clients.end()) {
                    // if the user didn't even try to log on a card in the current session 
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -1 : The client is not authentified");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                ClientTerminal* currClient = clients[i];
                if (currClient->getStatus() != 1) { // CLIENT_CONNECTED = 1
                    // if this client is not logged on any card
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -1 : The client is not authentified");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                if (cards.find(destCardNo) == cards.end()) {
                    // if the destination card does not exist
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -4 : Unexistent card number");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                Card* destCard = cards[destCardNo];

                Card* currCard = cards[currClient->getCurrCardNo()];
                if (currCard->getSum() < transferSum) {
                    // if the client does not have enough money on the currently connected card in order to make the transfer
                    memset(buffer, 0, BUFLEN);
                    sprintf(buffer, "IBANK> -8 : Insufficient funds");
                    send(i, buffer, sizeof(buffer), 0);
                    continue;
                }

                // mark the sum that it's meant to be transfered
                currClient->setPendingSum(transferSum);
                currClient->setStatus(2); // CLIENT_PENDINGTRANSFERAPPROVAL
                currClient->setDestCardNo(destCardNo);
                memset(buffer, 0, BUFLEN);
                sprintf(buffer, "IBANK> Transfer %.2lf to %s %s? [y/n]", transferSum, destCard->getLastName().c_str(), destCard->getFirstName().c_str());
                send(i, buffer, sizeof(buffer), 0);
                continue;
            }


            // --- quit
            if (strcmp(p, "quit") == 0) {

                close(i);
                FD_CLR(i, &read_fds);

                if (clients.find(i) == clients.end()) {
                    // if the user didn't even try to log on a card in the current session 
                    continue;
                }

                ClientTerminal* currClient = clients[i]; // pointer to the ClientTerminal connected on socket i
                if (currClient->getStatus() != 1) { // CLIENT_CONNECTED = 1
                    // if this client is not logged on any card
                    continue;
                }

                string cardNo = clients[i]->getCurrCardNo();
                if (cards.find(cardNo) != cards.end()) {
                    cards[cardNo]->setStatus(0);
                }
                clients.erase(i); // remove the client from the clients map. When he/she will log in again, they will be readded

                continue;
            }

            // --- any other input from the client is invalid
            memset(buffer, 0, BUFLEN);
            sprintf(buffer, "IBANK> -6 : Failed operation");
            send(i, buffer, sizeof(buffer), 0);
        }
    }
    close(fdUDP);
    close(fdListenTCP);
}