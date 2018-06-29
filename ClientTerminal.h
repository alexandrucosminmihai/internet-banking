#ifndef PROIECT_SERVER_CLIENTTERMINAL_H
#define PROIECT_SERVER_CLIENTTERMINAL_H

#define CLIENT_DISCONNECTED = 0;
#define CLIENT_CONNECTED = 1
#define CLIENT_PENDINGTRANSFERAPPROVAL = 2;

#include <string>

using namespace std;

class ClientTerminal {
private:
    string currCardNo;
    string destCardNo;
    int pinFailNo;
    int socketTCP;
    int status;
    double pendingSum;

public:

    ClientTerminal(string currCardNo, int pinFailNo, int socketTCP, int status);

    const string &getCurrCardNo() const;

    void setCurrCardNo(const string &currCardNo);

    int getPinFailNo() const;

    void setPinFailNo(int pinFailNo);

    int getSocketTCP() const;

    void setSocketTCP(int socketTCP);

    int getStatus() const;

    void setStatus(int status);

    double getPendingSum() const;

    void setPendingSum(double pendingSum);

    const string &getDestCardNo() const;

    void setDestCardNo(const string &destCardNo);
};


#endif //PROIECT_SERVER_CLIENTTERMINAL_H
