#include "ClientTerminal.h"

const string &ClientTerminal::getCurrCardNo() const {
    return currCardNo;
}

void ClientTerminal::setCurrCardNo(const string &currCardNo) {
    ClientTerminal::currCardNo = currCardNo;
}

int ClientTerminal::getPinFailNo() const {
    return pinFailNo;
}

void ClientTerminal::setPinFailNo(int pinFailNo) {
    ClientTerminal::pinFailNo = pinFailNo;
}

int ClientTerminal::getSocketTCP() const {
    return socketTCP;
}

void ClientTerminal::setSocketTCP(int socketTCP) {
    ClientTerminal::socketTCP = socketTCP;
}

ClientTerminal::ClientTerminal(string currCardNo, int pinFailNo, int socketTCP, int status) {
    this->currCardNo = currCardNo;
    this->pinFailNo = pinFailNo;
    this->socketTCP = socketTCP;
    this->status = status;
}

int ClientTerminal::getStatus() const {
    return status;
}

void ClientTerminal::setStatus(int status) {
    ClientTerminal::status = status;
}

double ClientTerminal::getPendingSum() const {
    return pendingSum;
}

void ClientTerminal::setPendingSum(double pendingSum) {
    ClientTerminal::pendingSum = pendingSum;
}

const string &ClientTerminal::getDestCardNo() const {
    return destCardNo;
}

void ClientTerminal::setDestCardNo(const string &destCardNo) {
    ClientTerminal::destCardNo = destCardNo;
}
