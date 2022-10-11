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

void write_int_to_bit_array(int givenInt, bool *array) {
    for (int i = 0; i < 8 * 4; ++i) {
        int value = (givenInt & (int) pow(2, i)) >> i;
        bool boolean = {value};
        array[i] = boolean;
    }
}

int get_index_of_last_nonnull_element_of_bit_array(const bool *array) {
    int lastTrueBitIndex = 0;
    for (int i = 39; i >= 0; --i) {
        if (array[i] == true) {
            lastTrueBitIndex = i;
            break;
        }
    }
    return lastTrueBitIndex;
}

void int_bitarray_to_varint_bitarray(bool *array, int last_nonnull_byte_index, int *size) {
    for (int i = 0; i < 5; ++i) {
        if (i <= last_nonnull_byte_index) {
            for (int j = 39; j > (i+1)*8-1; --j) {
                array[j] = array[j-1];
            }
            array[(i+1)*8-1] = true;
            (*size)++;
        } else {
            break;
        }
    }
}

void bitarray_to_bytearray(const bool *array, uint8_t *byteArray) {
    for (int i = 0; i < 5; ++i) {
        uint8_t byte = 0;
        for (int j = 0; j < 8; ++j) {
            byte += pow(2, j) * array[(i*8)+j];
        }
        byteArray[i] = byte;
    }
}

VarInt* writeVarInt(unsigned int givenInt) {
    VarInt* varInt = malloc(sizeof *varInt);
    bool array[40] = {0};
    write_int_to_bit_array(givenInt, array);

    int last_nonnull_bit_index = get_index_of_last_nonnull_element_of_bit_array(array);

    int last_nonnull_byte_index = (int) ceil(last_nonnull_bit_index / 8.0) - 1;
    int byte_array_size = 1;
    int_bitarray_to_varint_bitarray(array, last_nonnull_byte_index, &byte_array_size);

    uint8_t byteArray[5] = {0};

    bitarray_to_bytearray(array, byteArray);
    varInt->length = byte_array_size;
    memcpy(varInt->bytes, byteArray, byte_array_size);
    return varInt;
}

unsigned char* get_bytes(VarInt* varInt) {
    return varInt->bytes;
}

unsigned char get_length(VarInt* varInt) {
    return varInt->length;
}



