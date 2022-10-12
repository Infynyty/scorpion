//
// Created by Kasimir on 12.10.2022.
//

#ifndef CMC_BUFFER_H
#define CMC_BUFFER_H

#include <string.h>
#include <winsock2.h>
#include <errno.h>
#include <stdio.h>
#include "VarInt/MCVarInt.h"

typedef struct Buffer {
    char* bytes;
    size_t byte_size;
    char* current_byte;
} Buffer;

Buffer* buffer_new();
void buffer_free(Buffer* buffer);
void buffer_write(Buffer* buffer, void* bytes, const size_t length_in_bytes);
int buffer_send(const Buffer* buffer, const SOCKET socket);
int buffer_read_varint(Buffer* buffer);
void buffer_read_string(Buffer* buffer, char* string_destination);
void buffer_receive(Buffer* buffer, SOCKET socket, size_t length);


#endif //CMC_BUFFER_H
