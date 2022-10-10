//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdlib.h>
#include <string.h>
#include "VarInt.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>

struct VarInt {
    unsigned char length;
    int8_t* bytes;
};

typedef struct Boolean {
    uint8_t value : 1;
}__attribute__((packed)) Boolean;

VarInt* writeVarInt(int givenInt) {
    VarInt* varInt = malloc(sizeof *varInt);
    Boolean array[40] = {0};
    for (int i = 0; i < 8 * 4; ++i) {
        int value = (givenInt & (int) pow(2, i)) >> i;
        Boolean boolean = {value};
        array[i] = boolean;
    }

    int8_t byteArray[5] = {0};
    char i = (char) givenInt;
    for (int i = 0; i < 4; ++i) {
        int getBytes = givenInt & (0xff << 8*i);
        byteArray[i] = getBytes >> 8*i;
    }


    int lastTrueBitIndex = 0;

    for (int i = 4; i >= 0; --i) {
        if (byteArray[i] != 0) {
            lastTrueBitIndex = i;
            break;
        }
    }
    int inputSizeInBytes = lastTrueBitIndex;
    int outputSizeInBytes = 0;
    for (int i = 0; i < 5; ++i) {
        if(i <= inputSizeInBytes) {
            for (int j = 5; j >= i; --j) {
                byteArray[j] = (byteArray[j] >> 1) & (byteArray[j - 1] << 7);
            }
            byteArray[i] = byteArray[i] & 128;
            outputSizeInBytes++;
        }
    }
    memcpy(varInt->bytes, &byteArray, outputSizeInBytes);
    varInt->length = outputSizeInBytes;
    for (int i = 0; i < 5; ++i) {
        printf("%d", byteArray[i]);
    }
    return varInt;
}

unsigned char* get_bytes(VarInt* varInt) {
    return varInt->bytes;
}

unsigned char get_length(VarInt* varInt) {
    return varInt->length;
}



