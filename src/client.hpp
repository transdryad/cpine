#pragma once

#include <string>

enum State {
    NONE,
    STATUS,
    LOGIN,
    TRANSFER
};

class Client {
    public:
        int sockfd;
        State state;
        std::string username  = "";
        Client(int sockfd, State state);
};
