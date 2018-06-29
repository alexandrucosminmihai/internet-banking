#ifndef PROIECT_SERVER_CARD_H
#define PROIECT_SERVER_CARD_H

#define CARD_OFFLINE = 0;
#define CARD_ONLINE = 1;
#define CARD_LOCKED = 2;
#define CARD_UNLOCKREQUESTED = 3;

#include <string>
using namespace std;

class Card {
private:
    string lastName;
    string firstName;
    string cardNo;
    string pin;
    string password;
    double sum;
    int status; // the state of the card

public:
    Card(string lastName, string firstName, string cardNo, string pin, string password, double sum);

    const string &getLastName() const;

    void setLastName(const string &lastName);

    const string &getFirstName() const;

    void setFirstName(const string &firstName);

    const string &getCardNo() const;

    void setCardNo(const string &cardNo);

    const string &getPin() const;

    void setPin(const string &pin);

    const string &getPassword() const;

    void setPassword(const string &password);

    double getSum() const;

    void setSum(double sum);

    int getStatus() const;

    void setStatus(int status);
};


#endif //PROIECT_SERVER_CARD_H
