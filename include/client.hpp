#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <thread>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

class Client {
public:
    Client(const std::string& serverAddress, int port, const std::string& encryptionKey, const std::string& clientName);
    ~Client();

    void run();

private:
    std::string serverAddress_;
    int port_;
    std::string encryptionKey_;
    std::string clientName_;
    int socket_;

    void connectToServer();
    void communicateWithServer();
#ifdef _WIN32
    WSADATA wsaData;
#endif
};

#endif