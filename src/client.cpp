#include "src/client.hpp"

Client::Client(int sockfd, State state) {
    this->sockfd = sockfd;
    this->state = state;
}
