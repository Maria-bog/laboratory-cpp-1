#include "../include/client.hpp" 
#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Client::Client(int socket, const std::string& name, const std::string& key, bool is_admin)
    : socket(socket), name(name), key(key), admin(is_admin), connect(true) {}

void Client::send(const std::string& mess) const {
    std::string encrypted = encrypt(mess, key);
    ::send(socket, encrypted.c_str(), encrypted.size(), 0);
}

std::string Client::receive() const {
    char buffer[1024] = {0};
    ssize_t bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return "";
    return decrypt(std::string(buffer), key);
}

void Client::disconnect() {
    connect = false;
    close(socket);
}

std::string Client::encrypt(const std::string& text, const std::string& key) {
    std::string res = text;
    int keyLength = key.length();
    for (size_t i = 0; i < text.length(); i++) {
        char textC = text[i];
        char keyC = key[i % keyLength];
        if (isalpha(textC)) {
            char reg = islower(textC) ? 'a' : 'A';
            res[i] = (textC - reg + (keyC - reg) + 26) % 26 + reg;
        }
    }
    return res;
}

std::string Client::decrypt(const std::string& text, const std::string& key) {
    std::string res = text;
    int keyLength = key.length();
    for (size_t i = 0; i < text.length(); i++) {
        char textC = text[i];
        char keyC = key[i % keyLength];
        if (isalpha(textC)) {
            char reg = islower(textC) ? 'a' : 'A';
            res[i] = (textC - reg - (keyC - reg) + 26) % 26 + reg;
        }
    }
    return res;
}
