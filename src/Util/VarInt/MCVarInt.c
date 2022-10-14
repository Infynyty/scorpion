//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdlib.h>
#include <string.h>
#include "MCVarInt.h"
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

struct MCVarInt;

void write_int_to_bit_array(int givenInt, bool *array) {
    for (int i = 0; i < 8 * 4; ++i) {
        int value = (givenInt & (int) pow(2, i)) >> i;
        bool boolean = {value};
        array[i] = boolean;
    }
}

void int_bitarray_to_varint_bitarray(bool *array, int *size) {

    for (int i = 0; i < 5; ++i) {
        bool is_next_byte_used = false;
        for (int j = 0; j < 8; ++j) {
            if (array[((i+1)*8)+j]) {
                is_next_byte_used = true;
                break;
            }
        }
        if (is_next_byte_used) {
            for (int j = 39; j > (i+1)*8-1; --j) {
                array[j] = array[j-1];
            }
            array[(i*8)+7] = true;
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

MCVarInt* writeVarInt(unsigned int givenInt) {
    MCVarInt* varInt = malloc(sizeof *varInt);
    bool array[40] = {0};
    write_int_to_bit_array(givenInt, array);


    int byte_array_size = 1;
    int_bitarray_to_varint_bitarray(array, &byte_array_size);

    uint8_t byteArray[5] = {0};

    bitarray_to_bytearray(array, byteArray);
    varInt->length = byte_array_size;
    //TODO: fix
    memcpy(varInt->bytes, byteArray, 5);
    return varInt;
}

int varint_read(const char* bytes, int* byte_size) {
    *byte_size = 0;
    int result = 0;
    const int CONTINUE_BIT = 0b10000000;
    const int SEGMENT_BITS = 0b01111111;
    for (int i = 0; i < 5; ++i) {
        result += (*(bytes+i) & SEGMENT_BITS) << (8*i);
        (*byte_size)++;
        if (((*(bytes+i)) & CONTINUE_BIT) != (CONTINUE_BIT)) {
            break;
        }
    }
    return result;
}

int varint_receive(SOCKET socket) {
    char* current_byte = malloc(sizeof(char));
    int result = 0;
    const int CONTINUE_BIT = 0b10000000;
    const int SEGMENT_BITS = 0b01111111;
    for (int i = 0; i < 5; ++i) {
        recv(socket, current_byte, 1, 0);
        result += (*(current_byte) & SEGMENT_BITS) << (8*i);
        if (((*(current_byte)) & CONTINUE_BIT) != (CONTINUE_BIT)) {
            break;
        }
    }
    free(current_byte);
    return result;
}

uint8_t* get_bytes(MCVarInt* varInt) {
    return varInt->bytes;
}

unsigned char get_length(MCVarInt* varInt) {
    return varInt->length;
}



