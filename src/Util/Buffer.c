//
// Created by Kasimir on 12.10.2022.
//

#include "Buffer.h"

#define MAX_BUFFER_SIZE 2097151 // maximum length for a packet


Buffer* buffer_new() {
    Buffer* buffer = malloc(sizeof(Buffer));
    buffer->bytes = calloc(0, sizeof(char));
    buffer->byte_size = 0;
    buffer->current_byte = buffer->bytes;
    return buffer;
}

void buffer_free(Buffer* buffer) {
    if (buffer->current_byte != buffer->bytes) {
        free(buffer->current_byte);
    }
    free(buffer->bytes);
    free(buffer);
    buffer = NULL;
}

void buffer_write(Buffer* buffer, void* bytes, const size_t length_in_bytes) {
    if (buffer->byte_size + length_in_bytes >= MAX_BUFFER_SIZE) {
        fprintf(stderr, "Buffer length (%d) too big!", buffer->byte_size + length_in_bytes);
        free(buffer->bytes);
        exit(EXIT_FAILURE);
    }
    //Resize array
    char* temp = realloc(buffer->bytes, (buffer->byte_size + length_in_bytes) * sizeof(char));
    if (temp == NULL) {
        fprintf(stderr, "Reallocation failed!");
        free(buffer->bytes);
        exit(EXIT_FAILURE);
    } else {
        buffer->bytes = temp;
        buffer->current_byte = temp;
    }
    //Write new bytes to the last location of the old array
    memcpy(buffer->bytes + buffer->byte_size, bytes, length_in_bytes);
    //Set new size of array
    buffer->byte_size += length_in_bytes;
}

int buffer_send(const Buffer* buffer, const SOCKET socket) {
    return send(socket, buffer->bytes, (int) buffer->byte_size, 0);
}

void buffer_read(Buffer* buffer, char* bytes, size_t length) {
    memcpy(bytes, buffer->current_byte, length);
    buffer->current_byte += length;
}

int buffer_read_varint(Buffer* buffer) {
    int size = 0;
    int varint = varint_read(buffer->current_byte, &size);
    buffer->current_byte += size;
    return varint;
}

void buffer_read_string(Buffer* buffer, char* string_destination) {
    int string_length = buffer_read_varint(buffer);
    memcpy(string_destination, buffer->current_byte, string_length);
}

void buffer_receive(Buffer* buffer, SOCKET socket, size_t length) {
    char bytes[length];
    recv(socket, bytes, (int) length, 0);
    buffer_write(buffer, bytes, length);
}
