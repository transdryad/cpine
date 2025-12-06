#pragma once

#include <string>

#define SEGMENT_BITS 0x7f
#define CONTINUE_BIT 0x80

int readVarInt(int sockfd);
int sizeVarInt(int value);
void writeVarInt(int sockfd, int value);

int readUShort(int sockfd);

std::string readString(int sockfd);
int sizeString(std::string string);
void writeString(int sockfd, std::string string);
