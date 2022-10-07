//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_VARINT_H
#define CMC_VARINT_H

typedef struct VarInt VarInt;

unsigned char* readVarInt(int varInt);

VarInt* writeVarInt(int givenInt);

unsigned char* get_bytes(VarInt* varInt);

#endif //CMC_VARINT_H
