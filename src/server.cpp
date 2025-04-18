#include "../include/server.hpp"
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#endif

Server::Server() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed.");
    }
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        throw std::runtime_error("Failed to create socket.");
    }
}

Server::~Server() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void Server::start(int port) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket.");
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket.");
    }

    std::cout << "[SERVER] Server started on port " << port << std::endl;

    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket >= 0) {
            std::thread(&Server::handleClient, this, client_socket).detach();
        }
    }
}

void Server::stop() {
#ifdef _WIN32
    closesocket(server_socket);
#else
    close(server_socket);
#endif
    std::lock_guard<std::mutex> lock(mtx);
    clients.clear();
}

void Server::handleClient(int client_socket) {
    try {
        char buffer[1024];
#ifdef _WIN32
        int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#else
        ssize_t received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#endif
        if (received <= 0) {
#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
            return;
        }

        buffer[received] = '\0';
        std::istringstream iss(buffer);
        std::string name, key, pas;
        iss >> name >> key >> pas;

        std::unique_ptr<Client> client;

        if (pas == admin_pass) {
            client = std::make_unique<Client>(client_socket, name, key, pas, true);
            std::cout << "[SERVER] Admin " << name << " connected." << std::endl;
        } else {
            client = std::make_unique<Client>(client_socket, name, key, pas);
            std::cout << "[SERVER] " << name << " connected." << std::endl;
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            clients[name] = std::move(client);
        }

        // 🌟 Main loop: just relay raw encrypted messages
        while (true) {
            char msg_buffer[1024] = {0};
#ifdef _WIN32
            int msg_len = recv(client_socket, msg_buffer, sizeof(msg_buffer) - 1, 0);
#else
            ssize_t msg_len = recv(client_socket, msg_buffer, sizeof(msg_buffer) - 1, 0);
#endif
            if (msg_len <= 0 || clients[name]->isKicked()) break;

            std::string raw_message(msg_buffer, msg_len);

            // check if it's a command (unencrypted)
            if (!raw_message.empty() && raw_message[0] == '/') {
                processCommand(clients[name].get(), raw_message);
                continue;
            }

            // Broadcast raw encrypted message with sender tag
            {
                std::lock_guard<std::mutex> lock(mtx);
                for (auto& [other_name, other_client] : clients) {
                    if (other_name != name) {
                        other_client->send(name + ": " + raw_message);
                    }
                }
            }
        }
    } catch (...) {
        std::cerr << "[SERVER] Exception in handleClient\n";
    }

    std::lock_guard<std::mutex> lock(mtx);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getSocket() == client_socket) {
            std::cout << "[SERVER] " << it->first << " disconnected." << std::endl;
            clients.erase(it);
            break;
        }
    }

#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}


void Server::processCommand(Client* sender, const std::string& cmd) {
    std::istringstream iss(cmd);
    std::string command;
    iss >> command;

    if (command == "/admin") {
        std::string name, password;
        iss >> name >> password;

        if (password == admin_pass) {
            std::lock_guard<std::mutex> lock(mtx);
            clients[name] = std::make_unique<Client>(sender->getSocket(), name, sender->getKey(), sender->getPas(), true);
            sender->send("Admin registered successfully.");
        } else {
            sender->send("Incorrect password.");
        }
    } else if (command == "/kick") {
        if (!sender->isAdmin()) {
            sender->send("You don't have enough rights :) ");
            return;
        }

        std::string target_name;
        iss >> target_name;

        std::lock_guard<std::mutex> lock(mtx);
        auto it = clients.find(target_name);
        if (it != clients.end()) {
            it->second->send("You have been kicked by admin.");
            it->second->kick();
#ifdef _WIN32
            shutdown(it->second->getSocket(), SD_BOTH);
            closesocket(it->second->getSocket());
#else
            shutdown(it->second->getSocket(), SHUT_RDWR);
            close(it->second->getSocket());
#endif
            clients.erase(it);
        } else {
            sender->send("User not found.");
        }
    } else if (command == "/m") {
        std::string target_name;
        iss >> target_name;
        std::string message;
        std::getline(iss, message);
        if (message.empty()) {
            sender->send("Usage: /m <username> <message>");
            return;
        }
    
        std::lock_guard<std::mutex> lock(mtx);
        auto it = clients.find(target_name);
        if (it != clients.end()) {
            std::string full_message = "[Private] " + sender->getName() + ": " + message;
            it->second->send(full_message);
            sender->send("Message sent to " + target_name);
        } else {
            sender->send("User not found.");
        }
    } else {
        sender->send("Unknown command.");
    }
}
