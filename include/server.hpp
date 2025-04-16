#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include "client.hpp"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

class Server {
private:
    int server_socket;
    std::unordered_map<std::string, std::unique_ptr<Client>> clients; // хранение клиентов
    std::mutex mtx; //мьютекс для синхронизации доступа к общим данным
    std::string admin_pass = "930286";
    void handleClient(int client_socket);
    void processCommand(Client* sender, const std::string& cmd);

public:
    Server();
    ~Server();
    
    void start(int port);
    void stop();
    void registerAdmin(const std::string& password);
};

