#include "../include/client.hpp"

Client::Client(const std::string& serverAddress, int port, const std::string& encryptionKey, const std::string& clientName)
    : serverAddress_(serverAddress), port_(port), encryptionKey_(encryptionKey), clientName_(clientName), socket_(-1)
{
    #ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        // Handle error appropriately (throw exception, exit, etc.)
    }
    #endif
}

Client::~Client() {
    if (socket_ != -1) {
        #ifdef _WIN32
            closesocket(socket_);
        #else
            close(socket_);
        #endif
    }
    #ifdef _WIN32
        WSACleanup();
    #endif
}

void Client::connectToServer() {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        std::cerr << "Socket creation error\n";
        // Handle error
        return; // Важно: выйдите из функции, если сокет не создан
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port_);

    #ifdef _WIN32
        // Инициализируем Winsock (если еще не инициализировали где-то раньше - лучше вынести в конструктор Client)
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "WSAStartup failed: " << iResult << std::endl;
            closesocket(socket_);
            return;
        }

        if (WSAStringToAddressA((LPSTR)serverAddress_.c_str(), AF_INET, NULL, (SOCKADDR*)&serverAddress, (int*)sizeof(serverAddress)) != 0) {
            std::cerr << "Invalid address (WSAStringToAddress): " << WSAGetLastError() << std::endl;
            closesocket(socket_);
            WSACleanup();
            return;
        }


    #else
        if (inet_pton(AF_INET, serverAddress_.c_str(), &serverAddress.sin_addr) <= 0) {
            std::cerr << "Invalid address\n";
            close(socket_);
            return;
        }
    #endif

    if (connect(socket_, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Connection failed\n";
        #ifdef _WIN32
            closesocket(socket_);
            WSACleanup();
        #else
            close(socket_);
        #endif
        return;
    }

    std::cout << "Connected to server\n";

    #ifdef _WIN32
      WSACleanup(); // Очищаем Winsock, когда он больше не нужен
    #endif
}

void Client::communicateWithServer() {
    char buffer[1024] = {0};

    while (true) {
        std::string message;
        std::cout << clientName_ << ", Enter message: ";
        std::getline(std::cin, message);

        // Send message
        #ifdef _WIN32
        send(socket_, message.c_str(), message.length(), 0);
        #else
        write(socket_, message.c_str(), message.length());
        #endif

        // Receive response
        #ifdef _WIN32
        int bytesRead = recv(socket_, buffer, sizeof(buffer), 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "Receive failed\n";
            break;
        }
        #else
        int bytesRead = read(socket_, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            std::cerr << "Receive failed\n";
            break;
        }
        #endif

        if (bytesRead == 0) {
            std::cout << "Server disconnected\n";
            break;
        }

        std::string receivedMessage(buffer, 0, bytesRead);
        std::cout << clientName_ << " received: " << receivedMessage << std::endl;
    }
}

void Client::run() {
    connectToServer();
    communicateWithServer();
}