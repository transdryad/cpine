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
#include "src/types.hpp"


//#define MAX_PLAYERS 20

Server::Server(std::string ip, std::string port) {
    this->ip = ip;
    this->port = port;
}

std::string Server::getStatusString() {
    players = 0; // configure server status response
    for (Client c : clients) {
        if (c.state == LOGIN) ++players;
    }

    //"{\"version\": {\"name\": \"1.21.8\",\"protocol\": 772},\"players\": {\"max\": 20,\"online\": 0,},\"description\": {\"text\": \"Testing!\"},\"enforcesSecureChat\": false}"
    return "{\"version\": {\"name\": \""+version+"\",\"protocol\": "+std::to_string(protocol)+"},\"players\": {\"max\": "+std::to_string(max_players)+",\"online\": "+std::to_string(players)+",\"sample\":[{\"name\":\"thinkofdeath\",\"id\":\"4566e69f-c907-48ee-8d71-d7ba5aa00d20\"}]},\"description\": {\"text\": \""+description+"\"}, \"enforcesSecureChat\": false}";
    //std::string val = "";
    //for (int i = 0; i < 227; i++) {
    //    val += "0";
    //}
    //return val;
}

void Server::disconnectPlayer(int index) {
    close(clients[index].sockfd);
    clients.erase(clients.begin() + index);
    std::cout << "Disconnecting Client" << std::endl;
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
    
    //std::vector<Client>::iterator client_iterator = clients.begin();
    
    int client_index = 0;

    while (true) { //better way, multiple clients
        //std::cout << getStatusString() << std::endl;
        int csock = accept(sock, (struct sockaddr *)&client_addr, &addr_size);
        if (csock != -1) {
            clients.emplace_back(csock, NONE); //try new player
            fcntl(clients.back().sockfd, F_SETFL, O_NONBLOCK);
            std::cout << "New client: " << clients.back().sockfd << std::endl;
        }
        //TODO: tick
        //
        //if (client_iterator == clients.end()) client_iterator = clients.begin(); //loop over clients forever
        if (client_index >= clients.size()) client_index = 0; //new forever loop
        if (clients.empty()) continue;
        
        Client& client = clients[client_index];
        int cfd = client.sockfd;

        int count = recv(cfd, &cbuffer, 2, MSG_PEEK);
        if (count < 2) {
            if (count == 0 || (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                disconnectPlayer(client_index);
                //disconnect client, as no data.
            }
            continue;
        }
        //printf("%x, %x\n", cbuffer[0], cbuffer[1]);
        if ((cbuffer[0] & 0xFE) == 0xFE && cbuffer[1] == 0x01) { //legacy ping
            std::cout << "Received Legacy Server Ping!" << std::endl;
            unsigned char kick = 0xFF;
            send(cfd, &kick, 1, 0);
            disconnectPlayer(client_index);
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
            case 0x00: // handshake/status ping (why tho?) also many others?
                if (client.state == STATUS) { //status response send
                    std::string status = getStatusString();
                    writeVarInt(cfd, 1 + sizeString(status)); //send packet size
                    writeVarInt(cfd, 0x0); //packId for status response
                    writeString(cfd, status);
                    std::cout << "Sent status" << std::endl;
                } else {
                    int proc_version = readVarInt(cfd);
                    std::string address = readString(cfd);
                    int port = readUShort(cfd);
                    State intent = (State)readVarInt(cfd);
                    std::cout << "Handshake: pv - " << proc_version << ", addr - " << address << ", port - " << port << ", intent - " << intent << std::endl;
                    client.state = intent;
                }
                break;
            case 0x01: //ping pong
                if (client.state == STATUS) {
                    printf("pong\n");
                    long long val = readLong(cfd);
                    //printf("%lld", val);
                    writeVarInt(cfd, sizeVarInt(0x01) + 8);
                    writeVarInt(cfd, 0x01); //packID
                    writeLong(cfd, val);
                    disconnectPlayer(client_index);
                }
                break;
        }
        ++client_index;
    }

    for (Client c : clients) {
        close(c.sockfd);
    }

    close(sock);
    freeaddrinfo(servinfo);
    freeaddrinfo(client_addr);
    return 0;
}
