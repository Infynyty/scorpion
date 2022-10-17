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

/**
 * A network buffer struct contains an array of chars (referred to as bytes), a size_t containing the length of the array and a char
 * pointer pointing to the current byte of the byte array.
 */
typedef struct NetworkBuffer {
    char *bytes;
    size_t byte_size;
    char *current_byte;         //TODO: remove?
} NetworkBuffer;

/**
 * Creates a new buffer with size 1 and an empty byte array.
 * @return A pointer to the new network buffer.
 */
NetworkBuffer *buffer_new();

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
 * @return Returns the result of the send_wrapper() function, i.e. the number of bytes sent, if the call was successful or an
 * error code.
 *
 * @warning The packet will sent alongside a header containing the buffer size as a VarInt.
 */
int buffer_send_packet(const NetworkBuffer *buffer, SOCKET socket);

/**
 * Reads a string from the incoming byte stream and writes its content to a buffer.
 * @param buffer    The buffer that the string will be written to.
 * @param socket    The socket from which the input will be taken.
 */
void buffer_read_string(NetworkBuffer *buffer, SOCKET socket);

/**
 * Attempts to print a buffer as a string.
 * @param buffer The contents of the buffer will be interpreted as a string.
 */
void buffer_print_string(NetworkBuffer* buffer);

/**
 * Takes bytes from an input stream and writes them to a buffer.
 * @param buffer The buffer that will be written to.
 * @param socket The socket from which the input will be taken.
 * @param length The amount of bytes that will be read from the input stream.
 */
void buffer_receive(NetworkBuffer *buffer, SOCKET socket, size_t length);

uint64_t buffer_receive_uint64(SOCKET socket);


#endif //CMC_NETWORKBUFFER_H
