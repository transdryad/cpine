#pragma once

#include <vector>
#include "src/client.hpp"

class Server {
    public:
        Server(std::string ip, std::string port, bool debug);
        ~Server();
        std::string ip;
        std::string port;
        int init();
        int run();
        int sock;
        int acceptClient();
        bool checkLegacyPing();
        bool checkDisconnect();
        bool debug;
        int handlePacket(int packid);
        int client_index = 0;
        struct addrinfo *client_addr;
        std::string version = "1.21.10";
        int protocol = 773;
        int max_players = 20;
        int players = 0;
        std::string description = "Testing!";
        //std::string status_str = "{\"version\": {\"name\": \"1.21.8\",\"protocol\": 772},\"players\": {\"max\": 20,\"online\": 0,},\"description\": {\"text\": \"Testing!\"]},\"enforcesSecureChat\": false}";
        std::string getStatusString();
        void disconnectClient();
        std::vector<Client> clients;
};
