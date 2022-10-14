//
// Created by Kasimir on 12.10.2022.
//

#ifndef CMC_NETWORKBUFFER_H
#define CMC_NETWORKBUFFER_H

#include <string.h>
#include <winsock2.h>
#include <errno.h>
#include <stdio.h>
#include "VarInt/MCVarInt.h"

typedef struct NetworkBuffer {
    char* bytes;
    size_t byte_size;
    char* current_byte;
} NetworkBuffer;

NetworkBuffer* buffer_new();
void buffer_free(NetworkBuffer* buffer);
void buffer_write(NetworkBuffer* buffer, void* bytes, const size_t length_in_bytes);
void buffer_write_little_endian(NetworkBuffer* buffer, void* bytes, const size_t length_in_bytes);
int buffer_send(const NetworkBuffer* buffer, const SOCKET socket);
int buffer_read_varint(NetworkBuffer* buffer);
void buffer_read_string(NetworkBuffer* buffer, char* string_destination);
void buffer_receive(NetworkBuffer* buffer, SOCKET socket, size_t length);


#endif //CMC_NETWORKBUFFER_H
