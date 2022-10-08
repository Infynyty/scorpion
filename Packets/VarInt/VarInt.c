//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdlib.h>
#include <string.h>
#include "VarInt.h"
#include <stdio.h>
#include <byteswap.h>

struct VarInt {
    unsigned char length;
    unsigned char* bytes;
};

int swap_int32( int val ) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

VarInt* writeVarInt(int givenInt) {

    VarInt* varInt = malloc(sizeof *varInt);
    int example = 50;

    int SEGMENT_BITS = 0x7F;
    int CONTINUE_BIT = 0x80;

    memcpy(varInt->bytes, calloc(1, sizeof(int)), (int) sizeof(get_length(varInt)) * sizeof(int));


    char array[5] = {0};
    for (int i = 0; i < 5; ++i) {
        int values = swap_int32(SEGMENT_BITS >> (i * 8) & givenInt);
        memcpy(array, &values, sizeof(int));
        if (array[0] & CONTINUE_BIT) {
            for (int j = 0; j < i; ++j) {
                array[j] = array[j+1];
            }
            char shifted = array[0] << 7;
            char leftover = array[0] | CONTINUE_BIT;
            array[0] = leftover;
            array[1] = leftover;
        }
    }
    memcpy(varInt->bytes, &example, sizeof( int));

    varInt->length = sizeof(int);

//
//    varInt->length = sizeof(int);
//    memcpy(varInt->bytes, calloc(1, sizeof(int)), (int) sizeof(get_length(varInt)) * sizeof(int));

    return varInt;
}

unsigned char* get_bytes(VarInt* varInt) {
    return varInt->bytes;
}

unsigned char* get_length(VarInt* varInt) {
    return &varInt->length;
}



