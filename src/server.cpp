#include "../include/server.hpp"
#include "../include/client.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>

Server::Server() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        throw std::runtime_error("Failed to create socket.");
    }
}

Server::~Server() {
    stop();
}

void Server::start(int port) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    //Привязка сокета к адресу и порту
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
    close(server_socket);
    std::lock_guard<std::mutex> lock(mtx);
    clients.clear();
}

void Server::handleClient(int client_socket) {
    try {
        // Чтение имени и ключа
        char buffer[1024];
        ssize_t received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            close(client_socket);
            return;
        }
        buffer[received] = '\0';
        std::istringstream iss(buffer);
        std::string name, key;
        iss >> name >> key;

        std::unique_ptr<Client> client;

        if (name == "admin" && key == admin_pass) {
            client = std::make_unique<Client>(client_socket, name, key, true);
            std::cout << "[SERVER] Admin connected." << std::endl;
        } else {
            client = std::make_unique<Client>(client_socket, name, key);
            std::cout << "[SERVER] " << name << " connected." << std::endl;
        }


        {
            std::lock_guard<std::mutex> lock(mtx);
            clients[name] = std::move(client);
        }

        std::cout << "[SERVER] " << name << " connected" << std::endl;

        while (true) {
            std::string message = clients[name]->receive();
            if (message.empty()) break;

            if (message[0] == '/') {
                processCommand(clients[name].get(), message);
            } else {
                auto broadcast = [this, &name, &message]() {
                    std::lock_guard<std::mutex> lock(mtx);
                    for (auto& [other_name, other_client] : clients) {
                        if (other_name != name) {
                
                            other_client->send(name + ": " + message);  
                        }
                    }
                };
                broadcast();
            }
        }
    } catch (...) {
    }

    std::lock_guard<std::mutex> lock(mtx);
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second->getSocket() == client_socket) {
            std::cout << "[SERVER] " << it->first << " disconnected." << std::endl;
            clients.erase(it);
            break;
        }
    }
    close(client_socket);
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
            clients[name] = std::make_unique<Client>(sender->getSocket(), name, sender->getKey(), true);
            sender->send("Admin registered successfully.");
        } else {
            sender->send("");
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
            close(it->second->getSocket());
            clients.erase(it);
        } else {
            sender->send("User not found.");
        }
    } else {
        sender->send("Unknown command.");
    }
}
