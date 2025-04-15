#include "../include/client.hpp"
#include <iostream>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error\n";
        return -1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return -1;
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed\n";
        close(sock);
        return -1;
    }
    

    std::string name, key, pas;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    std::cout << "Enter encryption key: ";
    std::getline(std::cin, key);
    std::cout << "Enter password: ";
    std::getline(std::cin, pas);

    std::string auth_info = name + " " + key + " " + pas;
    send(sock, auth_info.c_str(), auth_info.length(), 0);


    Client client(sock, name, key, pas);


    std::thread listener([&client]() {
        while (client.isConnected()) {
            std::string msg = client.receive();
            if (!msg.empty()) std::cout << "\n" << msg << std::endl;
        }
    });

    while (client.isConnected()) {
        std::string message;
        std::getline(std::cin, message);
        if (message == "/exit") {
            client.disconnect();
            break;
        }
        client.send(message);
    }

    listener.join();
    return 0;
}
