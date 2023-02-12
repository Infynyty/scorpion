//
// Created by Kasimir on 12.11.2022.
//

#include <stdint.h>
#include <stdbool.h>
#include "NBTParser.h"
#include "NetworkBuffer.h"
#include "Logger.h"

typedef enum NBT_TAGS {
	TAG_END, TAG_BYTE, TAG_SHORT, TAG_INT, TAG_LONG, TAG_FLOAT, TAG_DOUBLE, TAG_BYTE_ARRAY, TAG_STRING, TAG_LIST,
	TAG_COMPOUND, TAG_INT_ARRAY, TAG_LONG_ARRAY
} NBT_TAGS;

void handle_compound();

void handle_string();

void consume_nbt_data(NetworkBuffer *buffer) {
//    NetworkBuffer *test = buffer_new();
//    buffer_receive(test, socket, 200);

	NBT_TAGS element_tag = buffer_read(uint8_t, buffer);
	handle_string(buffer);
	handle_compound(buffer);
}

void handle_end(NetworkBuffer *buffer) {
	return;
}

void handle_byte(NetworkBuffer *buffer) {
	buffer_read(uint8_t, buffer);
}

void handle_short(NetworkBuffer *buffer) {
	buffer_read(uint16_t, buffer);
}

void handle_int(NetworkBuffer *buffer) {
	buffer_read(uint32_t, buffer);
}

void handle_long(NetworkBuffer *buffer) {
	buffer_read(uint64_t, buffer);
}

void handle_float(NetworkBuffer *buffer) {
	buffer_read(float, buffer);
}

void handle_double(NetworkBuffer *buffer) {
	buffer_read(double, buffer);
}

void handle_byte_array(NetworkBuffer *buffer) {
	int32_t length = buffer_read(int32_t, buffer);
	for (int i = 0; i < length; ++i) {
		handle_byte(buffer);
	}
}

void handle_string(NetworkBuffer *buffer) {
    NetworkBuffer *string_length = buffer_new();
    buffer_move(buffer, sizeof(uint16_t), string_length);
    buffer_swap_endianness(string_length);
    buffer_remove(buffer, (uint16_t) *string_length->bytes);
}

void handle_int_array(NetworkBuffer *buffer) {
	int32_t length = buffer_read(int32_t, buffer);
	for (int i = 0; i < length; ++i) {
		handle_int(buffer);
	}
}

void handle_long_array(NetworkBuffer *buffer) {
	int32_t length = buffer_read(int32_t, buffer);
	for (int i = 0; i < length; ++i) {
		handle_long(buffer);
	}
}


void handle_list(NetworkBuffer *buffer) {
	NBT_TAGS tag = buffer_read(uint8_t, buffer);
    NetworkBuffer *length_buf = buffer_new();
    buffer_move(buffer, sizeof(int32_t), length_buf);
    buffer_swap_endianness(length_buf);
	int32_t length = (int32_t) *length_buf->bytes;
	for (int i = 0; i < length; ++i) {
		switch (tag) {
			case TAG_END:
				handle_end(buffer);
				return;
			case TAG_BYTE:
				handle_byte(buffer);
				break;
			case TAG_SHORT:
				handle_short(buffer);
				break;
			case TAG_INT:
				handle_int(buffer);
				break;
			case TAG_LONG:
				handle_long(buffer);
				break;
			case TAG_FLOAT:
				handle_float(buffer);
				break;
			case TAG_DOUBLE:
				handle_double(buffer);
				break;
			case TAG_BYTE_ARRAY:
				handle_byte_array(buffer);
				break;
			case TAG_STRING:
				handle_string(buffer);
				break;
			case TAG_LIST:
				handle_list(buffer);
				break;
			case TAG_COMPOUND:
				handle_compound(buffer);
				break;
			case TAG_INT_ARRAY:
				handle_int_array(buffer);
				break;
			case TAG_LONG_ARRAY:
				handle_long_array(buffer);
				break;
		}
	}
}

void handle_compound(NetworkBuffer *buffer) {
	while (1) {
		NBT_TAGS element_tag = buffer_read(uint8_t, buffer);
		if (element_tag == TAG_END) return;
		handle_string(buffer);
		switch (element_tag) {
			case TAG_END:
				handle_end(buffer);
				return;
			case TAG_BYTE:
				handle_byte(buffer);
				break;
			case TAG_SHORT:
				handle_short(buffer);
				break;
			case TAG_INT:
				handle_int(buffer);
				break;
			case TAG_LONG:
				handle_long(buffer);
				break;
			case TAG_FLOAT:
				handle_float(buffer);
				break;
			case TAG_DOUBLE:
				handle_double(buffer);
				break;
			case TAG_BYTE_ARRAY:
				handle_byte_array(buffer);
				break;
			case TAG_STRING:
				handle_string(buffer);
				break;
			case TAG_LIST:
				handle_list(buffer);
				break;
			case TAG_COMPOUND:
				handle_compound(buffer);
				break;
			case TAG_INT_ARRAY:
				handle_int_array(buffer);
				break;
			case TAG_LONG_ARRAY:
				handle_long_array(buffer);
				break;
		}
	}
}