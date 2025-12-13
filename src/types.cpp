#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

#include "src/types.hpp"

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

int sizeVarInt(int value) {
    int size = 1;
    while ((value & ~SEGMENT_BITS) != 0) {
        value >>= 7;
        size++;
    }
    return size;
}

void writeVarInt(int sockfd, int value) {
    while (true) {
        if ((value & ~SEGMENT_BITS) == 0) {
            char ch = value;
            send(sockfd, &ch, 1, 0);
            return;
        }
        char ch = (value & SEGMENT_BITS) | CONTINUE_BIT;
        send(sockfd, &ch, 1, 0);
        value = value >> 7;
    }
}

int readUShort(int sockfd) {
    char bytes[2];
    recv(sockfd, &bytes, 2, 0);
    unsigned short native = ((unsigned short)bytes[1] << 8) | bytes[0];
    native = ntohs(native);
    return (int)native;
}

long long readLong(int sockfd) {
    char bytes[8];
    recv(sockfd, &bytes, 8, 0);
    long long value;
    memcpy(&value, bytes, sizeof(value));
    //std::cout << value << std::endl;
    value = be64toh(value);
    //std::cout << value << std::endl;
    return value;
}

void writeLong(int sockfd, long long value) {
    //std::cout << value << std::endl;
    value = htobe64(value);
    //std::cout << value << std::endl;
    //char bytes[8] = reinterpret_cast<char[]>(value);
    char bytes[8];
    memcpy(bytes, &value, sizeof(value));
    //const char *bytes = reinterpret_cast<char const *>(value);
    /*std::cout << "Long to send: " << std::endl;
    for (int i = 0; i < 8; i++) {
        printf("%x, ", bytes[i]);
    } */
    send(sockfd, bytes, 8, 0);
}
/*
struct uint128 readUUID(int sockfd) {
    struct uint128 value;

}

void writeUUID(int sockfd, struct uint128 value) {
    
}
*/
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

int sizeString(std::string string) {
    return sizeVarInt(strlen(string.c_str())) + strlen(string.c_str());
}

void writeString(int sockfd, std::string string) {
    auto cstr = string.c_str();
    writeVarInt(sockfd, strlen(cstr));
    //std::cout << strlen(cstr) << ": " << cstr << std::endl;
    if (send(sockfd, cstr, strlen(cstr), 0) < strlen(cstr)) {
        exit(1);
    } 
}
