#pragma once

#include <string>

#define SEGMENT_BITS 0x7f
#define CONTINUE_BIT 0x80

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif


int readVarInt(int sockfd);
int sizeVarInt(int value);
void writeVarInt(int sockfd, int value);

int readUShort(int sockfd);

long long readLong(int sockfd);
void writeLong(int sockfd, long long value);

std::string readString(int sockfd);
int sizeString(std::string string);
void writeString(int sockfd, std::string string);
