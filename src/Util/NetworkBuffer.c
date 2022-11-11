//
// Created by Kasimir on 12.10.2022.
//

#include <stdbool.h>
#include <stdlib.h>
#include "NetworkBuffer.h"
#include "SocketWrapper.h"

#define MAX_BUFFER_SIZE 2097151 // maximum length for a packet


NetworkBuffer *buffer_new() {
	NetworkBuffer *buffer = malloc(sizeof(NetworkBuffer));
	buffer->bytes = calloc(0, sizeof(char));
	buffer->byte_size = 0;
	return buffer;
}

void buffer_free(NetworkBuffer *buffer) {
	free(buffer->bytes);
	free(buffer);
	buffer = NULL;
}

void buffer_write_bytes(NetworkBuffer *buffer, void *bytes, const size_t length_in_bytes) {
	if (buffer->byte_size + length_in_bytes >= MAX_BUFFER_SIZE) {
		fprintf(stderr, "NetworkBuffer length (%d) too big!", buffer->byte_size + length_in_bytes);
		free(buffer->bytes);
		exit(EXIT_FAILURE);
	}
	//Resize array
	char *temp = realloc(buffer->bytes, (buffer->byte_size + length_in_bytes) * sizeof(char));
	if (temp == NULL) {
		fprintf(stderr, "Reallocation failed!");
		free(buffer->bytes);
		exit(EXIT_FAILURE);
	} else {
		buffer->bytes = temp;
	}
	//Write new bytes to the last location of the old array
	memmove(buffer->bytes + buffer->byte_size, bytes, length_in_bytes);
	//Set new size of array
	buffer->byte_size += length_in_bytes;
}

void swap_endianness(void *bytes, const size_t length_in_bytes) {
	for (int low = 0, high = (int) length_in_bytes - 1; low < high; low++, high--) {
		char temp = (char) *((char *) bytes + low);
		*((char *) bytes + low) = (char) *((char *) bytes + high);
		*((char *) bytes + high) = temp;
	}
}

//TODO: check endianness at compile time
void buffer_swap_endianness(NetworkBuffer *buffer) {
	swap_endianness(buffer->bytes, buffer->byte_size);
}

void buffer_write(NetworkBuffer *buffer, void *bytes, const size_t length_in_bytes) {
	swap_endianness(bytes, length_in_bytes);
	buffer_write_bytes(buffer, bytes, length_in_bytes);
}

void buffer_write_little_endian(NetworkBuffer *buffer, void *bytes, const size_t length_in_bytes) {
	buffer_write_bytes(buffer, bytes, length_in_bytes);
}

void buffer_send_packet(const NetworkBuffer *buffer, SocketWrapper *socket) {
	// prefix packet with packet size as varint
	NetworkBuffer *packet_size_bytes = buffer_new();
	MCVarInt *packet_size_varint = writeVarInt(buffer->byte_size);
	buffer_write_little_endian(packet_size_bytes, packet_size_varint->bytes, packet_size_varint->length);
	send_wrapper(socket, packet_size_varint->bytes, packet_size_varint->length);
	buffer_free(packet_size_bytes);
    free(packet_size_varint);
	send_wrapper(socket, buffer->bytes, buffer->byte_size);
}

// Strings

void buffer_receive_string(NetworkBuffer *buffer, SocketWrapper *socket) {
	int string_length = varint_receive(socket);
	buffer_receive(buffer, socket, string_length);
	char string_terminator = '\0';
	buffer_write_bytes(buffer, &string_terminator, 1);
}

void buffer_print_string(NetworkBuffer *buffer) {
	printf("%s", buffer->bytes);
}

// Integers

uint8_t buffer_receive_uint8_t(SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	buffer_receive(buffer, socket, sizeof(char));
	uint8_t result = 0;
	result += *buffer->bytes;
	buffer_free(buffer);
	return result;
}

uint16_t buffer_receive_uint16_t(SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	buffer_receive(buffer, socket, sizeof(uint64_t));
	uint64_t result = 0;
	for (int i = sizeof(uint16_t) - 1; i >= 0; --i) {
		result += (*buffer->bytes) << 8 * i;
	}
	buffer_free(buffer);
	return result;
}

uint32_t buffer_receive_uint32_t(SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	buffer_receive(buffer, socket, sizeof(uint64_t));
	uint32_t result = 0;
	for (int i = sizeof(uint32_t) - 1; i >= 0; --i) {
		result += (*buffer->bytes) << 8 * i;
	}
	buffer_free(buffer);
	return result;
}

uint64_t buffer_receive_uint64(SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	buffer_receive(buffer, socket, sizeof(uint64_t));
	uint64_t result = 0;
	for (int i = sizeof(uint64_t) - 1; i >= 0; --i) {
		result += (*buffer->bytes) << 8 * i;
	}
	buffer_free(buffer);
	return result;
}

// floating point numbers

float buffer_receive_float(SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	buffer_receive(buffer, socket, sizeof(float));
	buffer_swap_endianness(buffer);
	float result = 0;
	for (int i = sizeof(float) - 1; i >= 0; --i) {
		result += (*buffer->bytes) << 8 * i;
	}
	buffer_free(buffer);
	return result;
}

double buffer_receive_double(SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	buffer_receive(buffer, socket, sizeof(double));
	buffer_swap_endianness(buffer);
	double result = 0;
	for (int i = sizeof(double) - 1; i >= 0; --i) {
		result += (*buffer->bytes) << 8 * i;
	}
	buffer_free(buffer);
	return result;
}

void buffer_receive(NetworkBuffer *buffer, SocketWrapper *socket, size_t length) {
	char bytes[length];
	receive_wrapper(socket, bytes, (int) length);
	buffer_write_bytes(buffer, bytes, length);
}
