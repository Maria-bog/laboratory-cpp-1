#include "../include/client.hpp"
#include <iostream>
#include <thread>
#include <cstring>

Client::Client(int socket, const std::string& name, const std::string& key, const std::string& pas, bool is_admin)
    : socket(socket), name(name), key(key), pas(pas), admin(is_admin), connect(true) {}

void Client::send(const std::string& mess) const {
    std::string encrypted = encrypt(mess, key);
#ifdef _WIN32
    ::send(socket, encrypted.c_str(), encrypted.size(), 0);
#else
    ::send(socket, encrypted.c_str(), encrypted.size(), 0);
#endif
}

std::string Client::receive() const {
    char buffer[1024] = {0};
#ifdef _WIN32
    int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
#else
    ssize_t bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
#endif
    if (bytes <= 0) return "";
    return decrypt(std::string(buffer), key);
}

void Client::disconnect() {
    if (socket != -1) {
#ifdef _WIN32
        closesocket(socket);
#else
        close(socket);
#endif
        socket = -1;
    }
}

bool Client::isKicked() const {
    return is_kicked;
}

void Client::kick() {
    is_kicked = true;
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
