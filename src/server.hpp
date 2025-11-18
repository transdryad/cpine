#pragma once

class Server {
    public:
        Server(std::string ip, std::string port);
        std::string ip;
        std::string port;
        int run();
};
