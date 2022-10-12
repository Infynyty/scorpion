//
// Created by Kasimir on 12.10.2022.
//

#include "Buffer.h"


Buffer* buffer_new() {
    Buffer* buffer = calloc(0, sizeof(Buffer));
    buffer->bytes = calloc(0, sizeof(char));
    buffer->byte_size = 0;
    buffer->current_byte = buffer->bytes;
    return buffer;
}

void buffer_free(Buffer* buffer) {
    free(buffer->bytes);
    free(buffer);
}

void buffer_write(Buffer* buffer, void* bytes, const size_t length_in_bytes) {
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

void buffer_read_string(Buffer* buffer, char* string) {
    int string_length = buffer_read_varint(buffer);
    memcpy(string, buffer->current_byte, string_length);
}
