//
// Created by Kasimir on 12.10.2022.
//

#include <stdbool.h>
#include <stdlib.h>
#include "NetworkBuffer.h"
#include "SocketWrapper.h"
#include "Logger.h"

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
	if (length_in_bytes == 0) return;
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
	swap_endianness(bytes, length_in_bytes);
}

void buffer_write_little_endian(NetworkBuffer *buffer, void *bytes, const size_t length_in_bytes) {
	buffer_write_bytes(buffer, bytes, length_in_bytes);
}

static void buffer_remove(NetworkBuffer *buffer, const size_t length) {
	int32_t size_after_remove = (int32_t) (buffer->byte_size - length);
	if (size_after_remove < 0) {
		size_after_remove = 0;
	}
	//Resize array
	memmove(buffer->bytes, buffer->bytes + length, size_after_remove);
	char *temp = realloc(buffer->bytes, size_after_remove * sizeof(char));
	if (temp == NULL) {
		fprintf(stderr, "Reallocation failed!");
		free(buffer->bytes);
		exit(EXIT_FAILURE);
	} else {
		buffer->bytes = temp;
	}
	buffer->byte_size = size_after_remove;
}

void buffer_poll(NetworkBuffer *buffer, const size_t length, void *dest) {
	if (length > buffer->byte_size) {
		cmc_log(ERR, "Tried polling %d bytes of a buffer with size %d", length, buffer->byte_size);
		exit(EXIT_FAILURE);
	}
	memmove(dest, buffer->bytes, length);
	buffer_remove(buffer, length);
}

void buffer_move(NetworkBuffer *src, const size_t length, NetworkBuffer *dest) {
	buffer_poll(src, length, dest);
	dest->byte_size += length;
}

// Strings


NetworkBuffer *string_buffer_new(char *string) {
	NetworkBuffer *buffer = buffer_new();
	buffer_write_little_endian(buffer, string, strlen(string));
	return buffer;
}

void buffer_receive_string(NetworkBuffer *buffer, SocketWrapper *socket) {
	int string_length = varint_receive(socket);
	buffer_receive(buffer, socket, string_length);
	char string_terminator = '\0';
	buffer_write_bytes(buffer, &string_terminator, 1);
}

void buffer_print_string(NetworkBuffer *buffer) {
	printf("%s", buffer->bytes);
}

int32_t buffer_read_varint(NetworkBuffer *buffer) {
	unsigned char current_byte = 0;
	int result = 0;
	const int CONTINUE_BIT = 0b10000000;
	const int SEGMENT_BITS = 0b01111111;
	for (int i = 0; i < 5; ++i) {
		current_byte = buffer_read(uint8_t, buffer);
		result += (current_byte & SEGMENT_BITS) << (8 * i - i);
		if ((current_byte & CONTINUE_BIT) != (CONTINUE_BIT)) {
			break;
		}
	}
	return result;
}

void buffer_read_array(NetworkBuffer *src, NetworkBuffer *dest) {
	uint32_t length = buffer_read_varint(src);
	buffer_write_bytes(dest, src->bytes, length);
	buffer_remove(src, length);
}

static int32_t compression_threshold;
static bool compression_enabled;

void buffer_receive(NetworkBuffer *buffer, SocketWrapper *socket, size_t length) {
	char bytes[length];
	int received_bytes = 0;
	while (1) {
		int response = receive_wrapper(socket, bytes, (int) length - received_bytes);
		if (response == -1) {
			break;
		}
		received_bytes += response;
		buffer_write_bytes(buffer, bytes, received_bytes);
		if (received_bytes == length) {
			break;
		}
	}
}
