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

std::string encrypt(const std::string& text, const std::string& key) {
    std::string res = text;
    int keyLength = key.length();
    for (size_t i = 0; i < text.length(); i++) {
        char textC = text[i];
        char keyC = key[i % keyLength]; // для повтора ключа
        if (isalpha(textC)) {
            char reg = islower(textC) ? 'a' : 'A';
            res[i] = (textC - reg + (keyC - reg) + 26) % 26 + reg; // + сдвиг
        }
    }
    return res;
}

std::string decrypt(const std::string& text, const std::string& key) {
    std::string res = text;
    int keyLength = key.length();
    for (size_t i = 0; i < text.length(); i++) {
        char textC = text[i];
        char keyC = key[i % keyLength]; // для повтора ключа
        if (isalpha(textC)) {
            char reg = islower(textC) ? 'a' : 'A';
            res[i] = (textC - reg - (keyC - reg) + 26) % 26 + reg;
        }
    }
    return res;
}

void client(int sock, const std::string& key, const std::string& clientName) {
    while (true) {
        std::string message;
        std::cout << clientName << ", Enter message: ";
        std::getline(std::cin, message);
        std::string encryptedMessage = encrypt(message, key);

#ifdef _WIN32
        send(sock, encryptedMessage.c_str(), encryptedMessage.length(), 0);
#else
        write(sock, encryptedMessage.c_str(), encryptedMessage.length());
#endif

        char buffer[1024] = {0};
#ifdef _WIN32
        int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
#else
        int bytesRead = read(sock, buffer, sizeof(buffer));
#endif
        if (bytesRead <= 0) break;

        std::string decryptedMessage = decrypt(std::string(buffer), key);
        std::cout << clientName << " received message: " << decryptedMessage << std::endl;
    }

#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return -1;
    }
#endif

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return -1;
    }

    struct sockaddr_in serv_addr;
    std::string key;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed\n";
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return -1;
    }

    std::cout << "Enter encryption key: ";
    std::cin >> key;
    std::cin.ignore(); // Игнорируем оставшийся символ новой строки

    // Запуск клиента в отдельном потоке
    std::thread t(client, sock, key, "Client");
    t.join();

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    return 0;
}

