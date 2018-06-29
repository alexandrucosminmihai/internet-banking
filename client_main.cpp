#define BUFLEN 256

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <arpa/inet.h>
#include <fstream>
#include <unistd.h>


using namespace std;

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr,"Usage : %s <ip_server> <port_server>\n", argv[0]);
        exit(-10);
    }

    int fdUDP, fdTCP;
    int rc;
    struct sockaddr_in serv_addr;
    fd_set read_fds;
    fd_set tmp_fds;
    char buffer[BUFLEN];

    // create the UDP and TCP sockets
    fdUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (fdUDP < 0) {
        cerr << "Error: client unable to open UDP socket\n";
        exit(-10);
    }
    fdTCP = socket(AF_INET, SOCK_STREAM, 0);
    if (fdTCP < 0) {
        cerr << "Error: client unable to open TCP socket\n";
        exit(-10);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);

    // connect to the server over TCP
    rc = connect(fdTCP, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (rc < 0) {
        cerr << "Error: client unable to connect to the server via TCP\n";
        exit(-10);
    }

    // adding the file descriptors used for multiplexing by the client
    FD_SET(0, &read_fds); // the STDIN file descriptor
    FD_SET(fdTCP, &read_fds); // the file descriptor for the TCP connection
    FD_SET(fdUDP, &read_fds); // the file descriptor for the UDP connection

    int fdMax = fdTCP;
    if (fdUDP > fdMax) {
        fdMax = fdUDP;
    }

    int isLogged = 0;
    int unlockRequested = 0;
    string fileName = "client-";
    fileName = fileName + to_string(getpid()) + ".log";
    ofstream logFile;
    logFile.open(fileName);
    int ok = 1;
    socklen_t slen = sizeof(serv_addr);
    unsigned int enable = 0;
    string lastCardNo = "$NONE$";

    while (ok) {
        tmp_fds = read_fds;
        rc = select(fdMax + 1, &tmp_fds, NULL, NULL, NULL);
        if (rc == -1) {
            cerr << "Error: function call to select(...) in the client's main loop\n";
            exit(-10);
        }

        for (int i = 0; i <= fdMax; ++i) {
            if (!FD_ISSET(i, &tmp_fds)) {
                continue;
            }

            memset(buffer, 0, sizeof(buffer));

            if (i == 0) {
                // the client received input from the keyboard
                fgets(buffer, BUFLEN, stdin);
                logFile << buffer;

                char aux[BUFLEN];
                strcpy(aux, buffer);

                aux[strlen(aux) -1] = '\0';



                // check if the client has to send the secret password over UDP
                if (unlockRequested == 2) {
                    /*
                     if the client received the approval to send the secret password, 
                     now I must have read the password from STDIN
                     */
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "%s %s", lastCardNo.c_str(), aux);
                    rc = sendto(fdUDP, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, slen);
                    if (rc < 0) {
                        cerr << "Error: client unable to send the secret password from over UDP\n";
                        exit(-10);
                    }
                    unlockRequested = 0; // reset the unlocking status
                    continue;
                }

                char *p;
                p = strtok(aux, " ");

                // otherwise, check the user commands

                // --- login (sent over TCP)
                if (strcmp(p, "login") == 0) {

                	// remember the last card number from this session that the user has tried to access
                    p = strtok(NULL, " ");
                    lastCardNo = string(p); 
                    
                    if (isLogged == 1) {
                        // if the user is already logged on a card, don't send any request to the server
                        logFile << "IBANK> -2 : Session already active\n";
                        cout << "IBANK> -2 : Session already active\n";
                        continue;
                    }

                    rc = send(fdTCP, buffer, sizeof(buffer), 0);
                    if (rc < 0) {
                        cerr << "Error: client unable to send message to the server over TCP\n";
                        exit(-10);
                    }

                    isLogged = 2; // marchez faptul ca este in starea de awaiting approval

                    continue;
                }

                // --- logout (sent over TCP)
                if (strcmp(p, "logout") == 0) {

                    rc = send(fdTCP, buffer, sizeof(buffer), 0);
                    if (rc < 0) {
                        cerr << "Error: client unable to send logout request over TCP\n";
                        exit(-10);
                    }
                    if (isLogged == 1) {
                        isLogged = 0; // change the client's state to disconnected
                        /*
                         reset the variable for the last card that the client tried to access 
                         because the session ended
                        */
                        lastCardNo = "$NONE$"; 
                    }

                    continue;
                }

                // --- unlock (sent over UDP)
                if (strcmp(p, "unlock") == 0) {
                    memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "unlock %s", lastCardNo.c_str());
                    rc = sendto(fdUDP, buffer, sizeof(buffer), 0, (struct sockaddr *) &serv_addr, slen);
                    if (rc < 0) {
                        cerr << "Error: client unable to send unlock request over UDP\n";
                        exit(-10);
                    }
                    /*
                     remember the fact that there was an unlock request sent to the server and that
                     the client now waits for approval to send the secret password
                    */
                    unlockRequested = 1;

                    continue;
                }

                // --- quit (sent over TCP)
                if (strcmp(p, "quit") == 0) {
                    rc = send(fdTCP, buffer, sizeof(buffer), 0);
                    if (rc < 0) {
                        cerr << "Error: client unable to send quit request over TCP\n";
                        exit(-10);
                    }
                    close(fdTCP);
                    close(fdUDP);
                    logFile.close();
                    ok = 0;
                    continue;
                }

                // --- listsold and transfer (sent over TCP)
                rc = send(fdTCP, buffer, sizeof(buffer), 0);
                if (rc < 0) {
                    cerr << "Error: client unable to send listsold / transfer request over TCP\n";
                    exit(-10);
                }

                continue;
            }

            if (i == fdUDP) {
                // the client received something from the server over UDP
                rc = recvfrom(fdUDP, buffer, sizeof(buffer), 0, NULL, &enable);
                if (rc < 0) {
                    cerr << "Error: client unable to receive message from server over UDP\n";
                    exit(-10);
                }
                logFile << buffer << "\n";
                cout << buffer << "\n";
                // check if the message from the server says to send the secret password
                char aux[BUFLEN];
                strcpy(aux, buffer);
                char *p = strtok(aux, " ");
                p = strtok(NULL, " ");
                if (strcmp(p, "Send") == 0) {
                    // if the client has requested to unlock a card
                    if (unlockRequested == 1) {
                        unlockRequested = 2;
                    }
                } else {
                    if (unlockRequested == 1) {
                        unlockRequested = 0; // reset the unlock status because unlocking failed
                    }
                }

                continue;
            }

            if (i == fdTCP) {
                // the client received something from the server over TCP
                rc = recv(fdTCP, buffer, sizeof(buffer), 0);
                if (rc < 0) {
                    cerr << "Error: client unable to receive message from server over TCP\n";
                    exit(-10);
                }
                if (rc == 0) {
                    // if the server has shut down
                    close(fdTCP);
                    close(fdUDP);
                    logFile.close();
                    ok = 0;
                    continue;
                }
                logFile << buffer << "\n";
                cout << buffer << "\n";

                // check if the client received a log in confirmation
                char aux[BUFLEN];
                strcpy(aux, buffer);
                char* p = strtok(aux, " ");
                p = strtok(NULL, " ");

                if (strcmp(p, "Welcome") == 0 && isLogged == 2) {
                    isLogged = 1;
                }

                continue;
            }

        }
    }

    return 0;
}