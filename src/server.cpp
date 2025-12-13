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

Server::Server(std::string ip, std::string port, bool debug) {
    this->ip = ip;
    this->port = port;
    this->debug = debug;
    clients.reserve(20);
}

Server::~Server() {
    for (Client c : clients) {
        close(c.sockfd);
    }
    close(sock);
    freeaddrinfo(client_addr);
}

std::string Server::getStatusString() {
    players = 0; // configure server status response
    for (Client c : clients) {
        if (c.state == LOGIN) ++players;
    }

    //"{\"version\": {\"name\": \"1.21.8\",\"protocol\": 772},\"players\": {\"max\": 20,\"online\": 0,},\"description\": {\"text\": \"Testing!\"},\"enforcesSecureChat\": false}"
    return "{\"version\": {\"name\": \""+version+"\",\"protocol\": "+std::to_string(protocol)+"},\"players\": {\"max\": "+std::to_string(max_players)+",\"online\": "+std::to_string(players)+"},\"description\": {\"text\": \""+description+"\"}, \"enforcesSecureChat\": false}";
    //std::string val = "";
    //for (int i = 0; i < 227; i++) {
    //    val += "0";
    //}
    //return val;
}

void Server::disconnectClient() {
    close(clients[client_index].sockfd);
    clients.erase(clients.begin() + client_index);
    if (debug) std::cout << "Disconnecting Client" << std::endl;
}

int Server::init() {
    std::cout << "Running on " << this->ip << ":" << this->port;

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(this->ip.c_str(), this->port.c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "gai error: " << gai_strerror(status); return 1;
    }

    sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
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
    freeaddrinfo(servinfo); 
    return 0;
}

int Server::acceptClient() {
    socklen_t addr_size = sizeof(client_addr);
    int csock = accept(sock, (struct sockaddr *)&client_addr, &addr_size);
    if (csock != -1) {
        clients.emplace_back(csock, NONE); //try new player
        fcntl(clients.back().sockfd, F_SETFL, O_NONBLOCK);
        if (debug) std::cout << "New client: " << clients.back().sockfd << std::endl;
    }
    return csock;
}

bool Server::checkDisconnect() {
    char cbuffer[2];
    int count = recv(clients.at(client_index).sockfd, &cbuffer, 2, MSG_PEEK);
    if (count < 2) {
        if (count == 0 || (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            disconnectClient();
            //disconnect client, as no data.
        }
        return true;
    }
    return false;
}

bool Server::checkLegacyPing() {
    char cbuffer[2];
    int count = recv(clients.at(client_index).sockfd, &cbuffer, 2, MSG_PEEK);
    if (count == 2 && (cbuffer[0] & 0xFE) == 0xFE && cbuffer[1] == 0x01) { //legacy ping
        if (debug) std::cout << "Received Legacy Server Ping!" << std::endl;
        unsigned char kick = 0xFF;
        send(clients.at(client_index).sockfd, &kick, 1, 0);
        disconnectClient();
        return true;
    }
    return false;
}

int Server::handlePacket(int packid) {
    int cfd = clients.at(client_index).sockfd;
    switch (clients.at(client_index).state) {
        case NONE: { // new
            if (packid == 0x00) { // handshake
                int proc_version = readVarInt(cfd);
                std::string address = readString(cfd);
                int port = readUShort(cfd);
                State intent = (State)readVarInt(cfd);
                if (debug) std::cout << "Handshake: pv - " << proc_version << ", addr - " << address << ", port - " << port << ", intent - " << intent << std::endl;
                clients.at(client_index).state = intent;
            } else { // ie wth is this?
                disconnectClient();
            }
            break;
        }
        case STATUS: { // status ping
            switch(packid) {
                case 0x00: { // status request
                    std::string status = getStatusString();
                    writeVarInt(cfd, 1 + sizeString(status)); //send packet size
                    writeVarInt(cfd, 0x0); //packId for status response
                    writeString(cfd, status);
                    if (debug) std::cout << "Sent status" << std::endl;
                    break;
                }
                case 0x01: {// ping/pong
                           //printf("pong\n");
                           //printf("%lld", val);
                    writeVarInt(cfd, sizeVarInt(0x01) + 8);
                    writeVarInt(cfd, 0x01); //packID
                    writeLong(cfd, readLong(cfd));
                    disconnectClient();
                    break;
                }
            }
            break;
        }
        case LOGIN: { // self-evident
            if (packid == 0x01) { // login start
                                  // login ig idk
            }
            break;
        }
        case TRANSFER: {
            break; // unsure why this exists
        }
    }
    return 0;
}

int Server::run() {
    char cbuffer[2048];

    while (true) { //better way, multiple clients
                   //if (debug) std::cout << getStatusString() << std::endl;

        acceptClient();

        //TODO: tick

        //if (client_iterator == clients.end()) client_iterator = clients.begin(); //loop over clients forever
        if (client_index >= (long int)clients.size()) client_index = 0; //new forever loop
        if (clients.empty()) continue;

        Client& client = clients.at(client_index);
        int cfd = client.sockfd;

        if (checkDisconnect()) continue;

        if (checkLegacyPing()) continue;

        if (debug) printf("First two: %x, %x\n", cbuffer[0], cbuffer[1]);

        int length = readVarInt(cfd);
        if (debug) std::cout << "Length: " << length << std::endl;
        recv(cfd, &cbuffer, length, MSG_PEEK);
        if (debug) {
            std::cout << "Raw Packet: ";
            for (int i = 0; i < length; i++) {
                std::cout << "0x0";
                printf("%x", cbuffer[i]);
                std::cout << ", ";
            }
            std::cout << std::endl;
        }

        int packid = readVarInt(cfd);
        if (debug) std::cout << "PackID: " << packid << std::endl;

        handlePacket(packid);

        ++client_index;
    }
    return 0;
}
