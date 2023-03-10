#include <stdlib.h>
#include "NetworkBuffer.h"
#include "SocketWrapper.h"
#include "Logger.h"

#define MAX_BUFFER_SIZE 2097151 // maximum size for a packet

NetworkBuffer *buffer_new() {
	NetworkBuffer *buffer = malloc(sizeof(NetworkBuffer));
	buffer->bytes = calloc(0, sizeof(char));
	buffer->size = 0;
	return buffer;
}

void buffer_free(NetworkBuffer *buffer) {
    if (buffer == NULL) return;
	free(buffer->bytes);
	free(buffer);
	buffer = NULL;
}

void buffer_write(NetworkBuffer *buffer, void *bytes, const size_t length_in_bytes) {
	if (length_in_bytes == 0) return;
	if (buffer->size + length_in_bytes >= MAX_BUFFER_SIZE) {
        sc_log(ERR, "NetworkBuffer size (%d) too big!", buffer->size + length_in_bytes);
		free(buffer->bytes);
		exit(EXIT_FAILURE);
	}
	//Resize array
	char *temp = realloc(buffer->bytes, (buffer->size + length_in_bytes) * sizeof(char));
	if (temp == NULL) {
        sc_log(ERR, "Reallocation failed!");
		free(buffer->bytes);
		exit(EXIT_FAILURE);
	} else {
		buffer->bytes = (uint8_t *) temp;
	}
	//Write new bytes to the last location of the old array
	memmove(buffer->bytes + buffer->size, bytes, length_in_bytes);
	//Set new size of array
	buffer->size += length_in_bytes;
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
	swap_endianness(buffer->bytes, buffer->size);
}

void buffer_remove(NetworkBuffer *buffer, const size_t length) {
	int32_t size_after_remove = (int32_t) (buffer->size - length);
	if (size_after_remove < 0) {
		size_after_remove = 0;
	}
	//Resize array
	memmove(buffer->bytes, buffer->bytes + length, size_after_remove);
	char *temp = realloc(buffer->bytes, size_after_remove);
	if (temp == NULL && size_after_remove > 0) {
		fprintf(stderr, "Reallocation failed!");
		free(buffer->bytes);
		exit(EXIT_FAILURE);
	}
    buffer->bytes = (uint8_t *) temp;
	buffer->size = size_after_remove;
}

void buffer_peek(NetworkBuffer *buffer, const size_t length, void *dest) {
    if (length > buffer->size) {
        sc_log(ERR, "Tried polling %d bytes of a buffer with size %d", length, buffer->size);
        exit(EXIT_FAILURE);
    }
    memmove(dest, buffer->bytes, length);
}

void buffer_poll(NetworkBuffer *buffer, const size_t length, void *dest) {
    buffer_peek(buffer, length, dest);
	buffer_remove(buffer, length);
}



NetworkBuffer *buffer_clone(NetworkBuffer *buffer) {
    NetworkBuffer *clone = buffer_new();
    buffer_write(clone, buffer->bytes, buffer->size);
    return clone;
}

void buffer_move(NetworkBuffer *src, const size_t length, NetworkBuffer *dest) {
    char *temp = malloc(length);
	buffer_poll(src, length, temp);
    buffer_write(dest, temp, length);
    free(temp);
}

// Strings


NetworkBuffer *string_buffer_new(char *string) {
	NetworkBuffer *buffer = buffer_new();
    buffer_write(buffer, string, strlen(string));
	return buffer;
}

void buffer_print_string(NetworkBuffer *buffer) {
    sc_log(INFO, "%s", buffer->bytes);
}

int32_t buffer_read_varint(NetworkBuffer *buffer) {
	unsigned char current_byte;
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

int32_t buffer_peek_varint(NetworkBuffer *buffer) {
    unsigned char current_byte;
    uint8_t *ptr = buffer->bytes;
    int result = 0;
    const int CONTINUE_BIT = 0b10000000;
    const int SEGMENT_BITS = 0b01111111;
    for (int i = 0; i < 5; ++i) {
        current_byte = *ptr;
        result += (current_byte & SEGMENT_BITS) << (8 * i - i);
        if ((current_byte & CONTINUE_BIT) != (CONTINUE_BIT)) {
            break;
        }
        ptr++;
    }
    return result;
}

void buffer_read_array(NetworkBuffer *src, NetworkBuffer *dest) {
	uint32_t length = buffer_read_varint(src);
	buffer_write(dest, src->bytes, length);
	buffer_remove(src, length);
}

NetworkBuffer *buffer_read_string(NetworkBuffer *buffer) {
	NetworkBuffer *string = buffer_new();
	int32_t length = buffer_read_varint(buffer);
	uint8_t temp[length];
	buffer_poll(buffer, length, temp);
    buffer_write(string, temp, length);
	return string;
}

void buffer_receive(NetworkBuffer *buffer, SocketWrapper *socket, size_t length) {
	char bytes[length];
	int received_bytes = 0;
	while (1) {
		int response = receive_wrapper(socket, bytes, (int) length - received_bytes);
		if (response == -1) {
			break;
		}
		received_bytes += response;
		buffer_write(buffer, bytes, received_bytes);
		if (received_bytes == length) {
			break;
		}
	}
}
