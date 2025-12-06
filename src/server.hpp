#pragma once

#include <vector>
#include "src/client.hpp"

class Server {
    public:
        Server(std::string ip, std::string port);
        std::string ip;
        std::string port;
        int run();
        int state = 0;
        std::string status_str = "{\"version\": {\"name\": \"1.21.8\",\"protocol\": 772},\"players\": {\"max\": 20,\"online\": 0,},\"description\": {\"text\": \"Testing!\"]},\"enforcesSecureChat\": false}";
        std::vector<Client> clients;
};
