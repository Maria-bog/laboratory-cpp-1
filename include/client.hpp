#pragma once
#include <string>

class Client {
protected:
    int socket;
    std::string name;
    bool is_kicked = false; 
    bool connect;
    bool admin;
    std::string key;
    std::string pas;

public:
    Client(int socket, const std::string& name, const std::string& key, const std::string& pas, bool admin = false);

    
    void send(const std::string& mess) const;  
    std::string receive() const;
    void ban(Client& target) const;
    void disconnect();
    bool isKicked() const;
    void kick(); 

    static std::string encrypt(const std::string& text, const std::string& key);
    static std::string decrypt(const std::string& text, const std::string& key);

    void sendPublicMessage(const std::string& mess) const;
    void sendPrivateMessage(const std::string& mess, const std::string& recipient) const;

    std::string getName() const { return name; }  
    int getSocket() const { return socket; }
    std::string getKey() const { return key; }
    std::string getPas() const { return pas; }
    bool isConnected() const { return connect; }
    bool isAdmin() const { return admin; }
};
