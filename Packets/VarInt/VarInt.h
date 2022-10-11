//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_VARINT_H
#define CMC_VARINT_H

#include <stdint.h>

typedef struct VarInt {
    char length;
    int8_t bytes[5];
} __attribute__((__packed__)) VarInt;

unsigned char* readVarInt(int varInt);

VarInt* writeVarInt(unsigned int givenInt);

int8_t* get_bytes(VarInt* varInt);

unsigned char get_length(VarInt* varInt);

#endif //CMC_VARINT_H
