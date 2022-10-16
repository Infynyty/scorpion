//
// Created by Kasimir on 12.10.2022.
//

#include <stdbool.h>
#include "NetworkBuffer.h"

#define MAX_BUFFER_SIZE 2097151 // maximum length for a packet


NetworkBuffer* buffer_new() {
    NetworkBuffer* buffer = malloc(sizeof(NetworkBuffer));
    buffer->bytes = calloc(0, sizeof(char));
    buffer->byte_size = 0;
    buffer->current_byte = buffer->bytes;
    return buffer;
}

void buffer_free(NetworkBuffer* buffer) {
    free(buffer->bytes);
    free(buffer);
    buffer = NULL;
}

void buffer_write_bytes(NetworkBuffer* buffer, void* bytes, const size_t length_in_bytes) {
    if (buffer->byte_size + length_in_bytes >= MAX_BUFFER_SIZE) {
        fprintf(stderr, "NetworkBuffer length (%d) too big!", buffer->byte_size + length_in_bytes);
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

void buffer_write(NetworkBuffer* buffer, void* bytes, const size_t length_in_bytes) {
    for (int low = 0, high = (int) length_in_bytes - 1; low < high; low++, high--)
    {
        char temp = (char) *((char*) bytes + low);
        *((char*) bytes + low) = (char) *((char*) bytes + high);
        *((char*) bytes + high) = temp;
    }
    buffer_write_bytes(buffer, bytes, length_in_bytes);
}

void buffer_write_little_endian(NetworkBuffer* buffer, void* bytes, const size_t length_in_bytes) {
    buffer_write_bytes(buffer, bytes, length_in_bytes);
}

int buffer_send_packet(const NetworkBuffer* buffer, const SOCKET socket) {
    // prefix packet with packet size as varint
    NetworkBuffer* packetSizeBytes = buffer_new();
    MCVarInt* packetSize = writeVarInt(buffer->byte_size);
    buffer_write_little_endian(packetSizeBytes, packetSize->bytes, packetSize->length);
    send(socket, (const char *) packetSize->bytes, packetSize->length, 0);
    buffer_free(packetSizeBytes);

    return send(socket, buffer->bytes, (int) buffer->byte_size, 0);
}

void buffer_read_string(NetworkBuffer* buffer, SOCKET socket) {
    int string_length = varint_receive(socket);
    buffer_receive(buffer, socket, string_length);
    char string_terminator = '\0';
    buffer_write_bytes(buffer, &string_terminator, 1);
}

void buffer_print_string(NetworkBuffer* buffer) {
    printf("%s", buffer->bytes);
}

uint64_t buffer_receive_uint64(SOCKET socket) {
    NetworkBuffer *buffer = buffer_new();
    buffer_receive(buffer, socket, sizeof(uint64_t));
    uint64_t result = 0;
    for (int i = sizeof(uint64_t) - 1; i >= 0; --i) {
        result += (*buffer->bytes) << 8 * i;
    }
    buffer_free(buffer);
    return result;
}

void buffer_receive(NetworkBuffer* buffer, SOCKET socket, size_t length) {
    char bytes[length];
    recv(socket, bytes, (int) length, 0);
    buffer_write_bytes(buffer, bytes, length);
}
