#include "../include/client.hpp"

int main() {
    std::string serverAddress = "127.0.0.1";
    int port = 8080;
    std::string encryptionKey = "mysecretkey";
    std::string clientName;

    std::cout << "Enter your client name: ";
    std::getline(std::cin, clientName);

    Client client(serverAddress, port, encryptionKey, clientName);
    client.run();

    return 0;
}