#include <iostream>
#include "Server.h"

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr,"Usage : %s <port_server> <users_data_file>\n", argv[0]);
        exit(-10);
    }

    // create a new Server object using the port number and data file given as arguments
    Server server(atoi(argv[1]), argv[2]); 
    server.main();

    return 0;
}