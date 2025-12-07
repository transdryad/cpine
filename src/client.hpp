#pragma once

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
        Client(int sockfd, State state);
};
