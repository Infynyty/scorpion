//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_MCVARINT_H
#define CMC_MCVARINT_H

#include <stdint.h>
#include <winsock2.h>

typedef struct MCVarInt {
    char length;
    uint8_t bytes[5];
} __attribute__((__packed__)) MCVarInt;

unsigned char* readVarInt(int varInt);

MCVarInt* writeVarInt(unsigned int givenInt);

uint8_t* get_bytes(MCVarInt* varInt);

unsigned char get_length(MCVarInt* varInt);
int varint_read(const char* bytes, int* byte_size);
int varint_receive(SOCKET socket);

#endif //CMC_MCVARINT_H
