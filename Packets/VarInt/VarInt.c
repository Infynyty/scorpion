//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdlib.h>
#include <string.h>
#include "VarInt.h"

struct VarInt {
    unsigned char length;
    unsigned char* bytes;
};

VarInt* writeVarInt(int givenInt) {
    VarInt* varInt = malloc(sizeof *varInt);
    varInt->length = sizeof(int);
    memcpy(varInt->bytes, calloc(1, sizeof(int)), sizeof(int));
    return varInt;
}

unsigned char* get_bytes(VarInt* varInt) {
    return varInt->bytes;
}

