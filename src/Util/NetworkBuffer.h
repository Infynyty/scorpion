//
// Created by Kasimir on 12.10.2022.
//

#ifndef CMC_NETWORKBUFFER_H
#define CMC_NETWORKBUFFER_H

#include "SocketWrapper.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "VarInt/MCVarInt.h"

/**
 * A network buffer struct contains an array of chars (referred to as bytes) and a size_t containing the length
 * of the array.
 */
typedef struct NetworkBuffer {
	uint8_t *bytes;
	size_t byte_size;
} NetworkBuffer;

/**
 * Creates a new buffer with size 1 and an empty byte array.
 * @return A pointer to the new network buffer.
 */
NetworkBuffer *buffer_new();

void buffer_poll(NetworkBuffer *buffer, const size_t length, void *dest);

#define buffer_read(type, _buffer) ({    \
        type _result = 0;                             \
        buffer_poll(_buffer, sizeof(type), &_result); \
        _result;                                   \
    })

#define buffer_receive_type(type) ({ \
        NetworkBuffer *_buffer = buffer_new();                             \
        type _result = 0;                                     \
        buffer_receive(_buffer, get_socket(), sizeof(type));               \
        buffer_swap_endianness(_buffer);                                    \
        memmove(&_result, _buffer->bytes, sizeof(type));\
        buffer_free(_buffer);         \
        _result;                             \
})

void buffer_swap_endianness(NetworkBuffer *buffer);

NetworkBuffer *string_buffer_new(char *string);

/**
 * Frees a given buffer alongside its bytes.
 * @param buffer The buffer to be freed.
 */
void buffer_free(NetworkBuffer *buffer);

/**
 * Writes bytes to a buffer <em>in big endian order</em>.
 * @param buffer The buffer to be written to.
 * @param bytes  The array of bytes to be written into the buffer.
 * @param length_in_bytes   The length of the byte array.
 *
 * @warning Swaps the byte order of the bytes that were input. Input bytes should therefore be in little endian order.
 */
void buffer_write(NetworkBuffer *buffer, void *bytes, size_t length_in_bytes);

/**
 * Writes bytes to a buffer <em>in little endian order</em>. This can be used to write VarInts / VarLongs to a buffer
 * because their bytes have to be sent in little endian order.
 * @param buffer The buffer to be written to.
 * @param bytes  The array of bytes to be written into the buffer.
 * @param length_in_bytes   The length of the byte array.
 */
void buffer_write_little_endian(NetworkBuffer *buffer, void *bytes, size_t length_in_bytes);

/**
 * Send the bytes of a buffer which should be a complete packet to the server.
 * @param buffer The buffer from which the bytes should be taken.
 * @param socket The socket to send_wrapper the packet from.
 *
 * @warning The packet will sent alongside a header containing the buffer size as a VarInt.
 */
void buffer_send_packet(const NetworkBuffer *buffer, SocketWrapper *socket);

/**
 * Reads a string from the incoming byte stream and writes its content to a buffer.
 * @param buffer    The buffer that the string will be written to.
 * @param socket    The socket from which the input will be taken.
 */
void buffer_receive_string(NetworkBuffer *buffer, SocketWrapper *socket);

/**
 * Attempts to print a buffer as a string.
 * @param buffer The contents of the buffer will be interpreted as a string.
 */
void buffer_print_string(NetworkBuffer *buffer);

/**
 * Takes bytes from an input stream and writes them to a buffer.
 * @param buffer The buffer that will be written to.
 * @param socket The socket from which the input will be taken.
 * @param length The amount of bytes that will be read from the input stream.
 */
void buffer_receive(NetworkBuffer *buffer, SocketWrapper *socket, size_t length);

int32_t buffer_read_varint(NetworkBuffer *buffer);


#endif //CMC_NETWORKBUFFER_H
