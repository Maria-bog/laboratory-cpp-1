#include <iostream>
#include <cstring>
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

void handleClient(int client_socket, int other_client_socket) {
    char buffer[1024] = {0};
    ssize_t bytes_read;

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        // Получение сообщения от клиента
#ifdef _WIN32
        bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
#else
        bytes_read = read(client_socket, buffer, sizeof(buffer));
#endif
        if (bytes_read <= 0) {
            std::cerr << "Connection lost\n";
            break;
        }

        // Отправка сообщения другому клиенту
#ifdef _WIN32
        send(other_client_socket, buffer, bytes_read, 0);
#else
        write(other_client_socket, buffer, bytes_read);
#endif
    }

#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
}

int main() {
    int server_fd, client_sockets[2];
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creating socket\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
        //Проверка привязки
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed\n";
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 1;
    }
        //listen: Переводит сокет в режим ожидания подключений.
    if (listen(server_fd, 2) < 0) {
        std::cerr << "Listen failed\n";
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 1;
    }
        //создание 2 сокетов. accept подключение от клиента и создает новый сокет для общения с ним
    for (int i = 0; i < 2; i++) {
        if ((client_sockets[i] = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Accept failed\n";
#ifdef _WIN32
            closesocket(server_fd);
            WSACleanup();
#else
            close(server_fd);
#endif
            return 1;
        }
        std::cout << "Client " << i + 1 << " connected!" << std::endl;
    }

    // Запуск потоков для обработки клиентов
    std::thread client1_thread(handleClient, client_sockets[0], client_sockets[1]);
    std::thread client2_thread(handleClient, client_sockets[1], client_sockets[0]);

    client1_thread.join();
    client2_thread.join();

#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif

    return 0;
}



