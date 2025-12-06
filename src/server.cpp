#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <cstdio>
#include <unistd.h>
#include <array>
#include <valarray>
#include <cstring>
#include <vector>
#include <cstdint>
#include <fcntl.h>

#include "src/server.hpp"
#include "src/client.hpp"

#define SEGMENT_BITS 0x7f
#define CONTINUE_BIT 0x80
//#define MAX_PLAYERS 20

Server::Server(std::string ip, std::string port) {
    this->ip = ip;
    this->port = port;
}

int readVarInt(int sockfd) {
    int value = 0;
    int position = 0;
    char currentByte;
    char buffer[1];
    while (true) {
        recv(sockfd, &buffer, 1, 0);
        currentByte = buffer[0];
        value |= (currentByte & SEGMENT_BITS) << position;
        position += 7;
        if (!(currentByte & CONTINUE_BIT)) break;
    }
    return value;
}

void writeVarInt(std::vector<char>& bytes, int value) {
    while (true) {
        if ((value & ~SEGMENT_BITS) == 0) {
            bytes.push_back(value);
            return;
        }
        bytes.push_back((value & SEGMENT_BITS) | CONTINUE_BIT);
        value = value >> 7;
    }
}

std::string readString(int sockfd) {
    int length = readVarInt(sockfd);
    std::string output = "";
    char buffer[1];
    for (int i = 0; i < length; i++) {
        recv(sockfd, &buffer, 1, 0);
        //std::cout << ch << std::endl;
        output += buffer[0];
    }
    return output;
}

int readUShort(int sockfd) {
    char bytes[2];
    recv(sockfd, &bytes, 2, 0);
    unsigned short native = ((unsigned short)bytes[1] << 8) | bytes[0];
    native = ntohs(native);
    return (int)native;
}

int Server::run() {
    std::cout << "Running on " << this->ip << ":" << this->port;
    
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    char cbuffer[2048];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(this->ip.c_str(), this->port.c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "gai error: " << gai_strerror(status); return 1;
    }

    int sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sock == -1) {
        perror("Error getting socket"); return 1;
    }

    std::cout << " on socket:" << sock << std::endl;

    fcntl(sock, F_SETFL, O_NONBLOCK);

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (bind(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        perror("Error binding to port"); return 1;
    }

    if (listen(sock, 10) == -1) {
        perror("Error listening on port"); return 1;
    }

    std::cout << "Listening on port " << port << std::endl;
    
    struct addrinfo *client_addr;
    socklen_t addr_size = sizeof(client_addr);

    clients.reserve(20);
    
    std::vector<Client>::iterator client_iterator = clients.begin();

    while (true) { //better way, multiple clients
        clients.emplace_back(accept(sock, (struct sockaddr *)&client_addr, &addr_size), NONE); //try new player
        if (clients.back().sockfd == -1) { //failed accept or no client
            clients.pop_back();
        } else {
            fcntl(clients.back().sockfd, F_SETFL, O_NONBLOCK);
            std::cout << "New client: " << clients.back().sockfd << std::endl;
        }
        //TODO: tick
        //
        if (client_iterator == clients.end()) client_iterator = clients.begin(); //loop over clients forever
        if (clients.empty() || client_iterator == clients.end()) continue;

        int cfd = (*client_iterator).sockfd;

        int count = recv(cfd, &cbuffer, 2, MSG_PEEK);
        if (count < 2) {
            if (count == 0 || (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                clients.erase(client_iterator);
                //disconnect client, as no data.
            }
            continue;
        }

        int length = readVarInt(cfd);
        std::cout << length << std::endl;
        recv(cfd, &cbuffer, length, MSG_PEEK);

        std::cout << "Raw Packet: ";
        for (int i = 0; i < length; i++) {
            std::cout << "0x0";
            printf("%x", cbuffer[i]);
            std::cout << ", ";
        }
        std::cout << std::endl;

        int packid = readVarInt(cfd);
        std::cout << packid << std::endl;

        switch (packid) {
            case 0x0: // handshake/status ping (why tho?)
                //if (length == 1) {}
                int proc_version = readVarInt(cfd);
                std::string address = readString(cfd);
                int port = readUShort(cfd);
                State intent = (State)readVarInt(cfd);
                std::cout << "Handshake: pv - " << proc_version << ", addr - " << address << ", port - " << port << ", intent - " << intent << std::endl;
                (*client_iterator).state = intent;
        }
        
    }

    for (Client c : clients) {
        close(c.sockfd);
    }

    close(sock);
    freeaddrinfo(servinfo);
    freeaddrinfo(client_addr);
    return 0;
}
