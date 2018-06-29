//
// Created by mac on 4/29/18.
//

#include "Card.h"

Card::Card(string lastName, string firstName, string cardNo, string pin, string password, double sum) {
    this->lastName = lastName;
    this->firstName = firstName;
    this->cardNo = cardNo;
    this->pin = pin;
    this->password = password;
    this->sum = sum;
}

const string &Card::getLastName() const {
    return lastName;
}

void Card::setLastName(const string &lastName) {
    Card::lastName = lastName;
}

const string &Card::getFirstName() const {
    return firstName;
}

void Card::setFirstName(const string &firstName) {
    Card::firstName = firstName;
}

const string &Card::getCardNo() const {
    return cardNo;
}

void Card::setCardNo(const string &cardNo) {
    Card::cardNo = cardNo;
}

const string &Card::getPin() const {
    return pin;
}

void Card::setPin(const string &pin) {
    Card::pin = pin;
}

const string &Card::getPassword() const {
    return password;
}

void Card::setPassword(const string &password) {
    Card::password = password;
}

double Card::getSum() const {
    return sum;
}

void Card::setSum(double sum) {
    Card::sum = sum;
}

int Card::getStatus() const {
    return status;
}

void Card::setStatus(int status) {
    Card::status = status;
}
