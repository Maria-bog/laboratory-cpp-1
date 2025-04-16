#include "../include/server.hpp"
#include <iostream>

int main() {
    try {
        Server server;
        int port = 8080;  
        server.start(port);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}