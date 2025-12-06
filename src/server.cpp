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

int readVarInt(std::vector<char>& bytes, bool eraseFlag) {
    int value = 0;
    int position = 0;
    int byteCounter = 0;
    char currentByte;
    while (true) {
        currentByte = bytes[byteCounter];
        value |= (currentByte & SEGMENT_BITS) << position;
        position += 7;
        ++byteCounter;
        if (!(currentByte & CONTINUE_BIT)) break;
    }
    //std::cout << "Position: " << position << std::endl;
    if (eraseFlag) bytes.erase(bytes.begin(), bytes.begin() + byteCounter);
    return value;
}

void writeVarInt(std::vector<char>& bytes, int value) {
    //int i = 0;
    //unsigned int uval = (unsigned int)value;
    while (true) {
        if ((value & ~SEGMENT_BITS) == 0) {
            bytes.push_back(value);
            return;
        }
        bytes.push_back((value & SEGMENT_BITS) | CONTINUE_BIT);
        value = value >> 7;
    }
}

std::string readString(std::vector<char>& bytes, bool eraseFlag) {
    int length = readVarInt(bytes, true);
    std::string output = "";
    for (int i = 0; i < length; i++) {
        output += bytes[i];
    }
    if (eraseFlag) bytes.erase(bytes.begin(), bytes.begin() + length);
    return output;
}

int readUShort(std::vector<char>& bytes, bool eraseFlag) {
    unsigned short native =  ((unsigned short)bytes[1] << 8) | bytes[0];
    native = ntohs(native);
    if (eraseFlag) bytes.erase(bytes.begin(), bytes.begin() + 2);
    return (int)native;
}

int Server::run() {
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
    
    int client = 0;

    while (true) { //better way, multiple clients
        clients.emplace_back(accept(sock, (struct sockaddr *)&client_addr, &addr_size), NONE); //try new player
        if (clients.back().sockfd == -1) { //failed accept or no client
            clients.pop_back();
        } else {
            fcntl(clients.back().sockfd, F_SETFL, O_NONBLOCK);
            std::cout << "New client: " << clients.back().sockfd << std::endl;
        }
        //TODO: tick
        int cfd = clients[client].sockfd;
        
        
    }

    for (Client c : clients) {
        close(c.sockfd);
    }
    /* old method - kept for notes on packet handling
    int client_sock;
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof(client_addr);
    client_sock = accept(sock, (struct sockaddr *)&client_addr, &addr_size);

    std::cout << "Client connected!" << std::endl;
    
    int max_size = 1024;
    char buffer[max_size];
    std::vector<char> bytes;
    std::vector<char> packet;
    int packlength;

    while (true) {
        while (true) { //get full packet into bytes.
            recv(client_sock, &buffer, max_size, 0);
            for (int i = 0; i < max_size; i++) {
                bytes.push_back(buffer[i]);
                buffer[i] = 0;
            }
            packlength = readVarInt(bytes, false);
            if (bytes.size() >= packlength + 1) { break; }
        }
        readVarInt(bytes, true);

        std::copy(bytes.begin(), bytes.begin() + packlength, std::back_inserter(packet));
        bytes.erase(bytes.begin(), bytes.begin() + packlength);

        std::cout << "Raw Packet: ";
        for (char c : packet) {
            std::cout << "0x";
            printf("%x", c);
            std::cout << ", ";
        }
        std::cout << std::endl;

        //std::cout << "Packet Length: " << readVarInt(packet, true) << std::endl;
        int packID;
        packID = readVarInt(packet, true);
        //std::cout << packID << std::endl;
        switch (packID) {
            case 0:
                std::cout << "Protocol Version: " << readVarInt(packet, true) << std::endl;
                std::cout << "Hostname: " << readString(packet, true) << std::endl;
                std::cout << "Port: " << readUShort(packet, true) << std::endl;
                status = readVarInt(packet, true);
                std::cout << "Intent (1 - status, 2 - login, 3 - transfer): " << status << std::endl;
                switch (status) {
                    case 1:
                        if (bytes[0] == 0) { //ping & pong
                            bytes.erase(bytes.begin(), bytes.begin() + 1);
                            packet.clear();
                            packet.push_back(0x00);
                            writeVarInt(packet, strlen(status_str.c_str()));

                        }
                }
        }
    }
    close(client_sock);
    */
    close(sock);
    freeaddrinfo(servinfo);
    freeaddrinfo(client_addr);
    return 0;
}
