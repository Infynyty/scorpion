//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdlib.h>
#include <string.h>
#include "VarInt.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

struct VarInt {
    char length;
    int8_t* bytes;
};

VarInt* writeVarInt(int givenInt) {
    VarInt* varInt = malloc(sizeof *varInt);
    bool array[40] = {0};
    for (int i = 0; i < 8 * 4; ++i) {
        int value = (givenInt & (int) pow(2, i)) >> i;
        bool boolean = {value};
        array[i] = boolean;
    }

    int lastTrueBitIndex = 0;

    for (int i = 39; i >= 0; --i) {
        if (array[i] == true) {
            lastTrueBitIndex = i;
            break;
        }
    }

    for (int i = 0; i < 5; ++i) {
        if ((i+1)*8 <= lastTrueBitIndex) {
            for (int j = 39; j > (i+1)*8-1; --j) {
                array[j] = array[j-1];
            }
            array[(i+1)*8-1] = true;
        } else {
            break;
        }
    }
    uint8_t byteArray[5] = {0};

    for (int i = 0; i < 5; ++i) {
        uint8_t byte = 0;
        for (int j = 0; j < 7; ++j) {
            byte += pow(2, j) * array[(i+1)*j];
        }
        byteArray[i] = byte;
    }


    char i = (char) givenInt;
    for (int i = 0; i < 4; ++i) {
        int getBytes = givenInt & (0xff << 8*i);
        byteArray[i] = getBytes >> 8*i;
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



