//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdlib.h>
#include <string.h>
#include "MCVarInt.h"
#include "../Logging/Logger.h"
#include "../SocketWrapper.h"
#include "../NetworkBuffer.h"
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#define BYTE_LENGTH_IN_BIT 8
#define MAX_VARINT_LENGTH_IN_BYTES 5

struct MCVarInt;

void write_int_to_bit_array(int givenInt, bool *array) {
	for (int i = 0; i < 8 * 4; ++i) {
		int value = (givenInt & (int) pow(2, i)) >> i;
		bool boolean = {value};
		array[i] = boolean;
	}
}

void int_bitarray_to_varint_bitarray(bool *array, int *size) {

	for (int current_byte = 0; current_byte < MAX_VARINT_LENGTH_IN_BYTES; ++current_byte) {
		bool are_next_bytes_used = false;
		for (int current_bit = (current_byte * BYTE_LENGTH_IN_BIT) + 7;
		     current_bit < MAX_VARINT_LENGTH_IN_BYTES * BYTE_LENGTH_IN_BIT; ++current_bit) {
			if (array[current_bit]) {
				are_next_bytes_used = true;
				break;
			}
		}
		if (are_next_bytes_used) {
			for (int j = MAX_VARINT_LENGTH_IN_BYTES * BYTE_LENGTH_IN_BIT - 1;
			     j > (current_byte + 1) * BYTE_LENGTH_IN_BIT - 1; --j) {
				array[j] = array[j - 1];
			}
			array[(current_byte * 8) + 7] = true;
			(*size)++;
		} else {
			break;
		}
	}
}

void bitarray_to_bytearray(const bool *array, uint8_t *byteArray) {
	for (int i = 0; i < 5; ++i) {
		uint8_t byte = 0;
		for (int j = 0; j < 8; ++j) {
			byte += pow(2, j) * array[(i * 8) + j];
		}
		byteArray[i] = byte;
	}
}

MCVarInt *writeVarInt(unsigned int givenInt) {
	MCVarInt *varInt = malloc(sizeof *varInt);
	bool array[40] = {0};
	write_int_to_bit_array(givenInt, array);


	int byte_array_size = 1;
	int_bitarray_to_varint_bitarray(array, &byte_array_size);

	uint8_t byteArray[5] = {0};

	bitarray_to_bytearray(array, byteArray);
	varInt->length = byte_array_size;
	//TODO: fix
	memcpy(varInt->bytes, byteArray, 5);
	return varInt;
}

int varint_receive(SocketWrapper *socket) {
	unsigned char current_byte = 0;
	int result = 0;
	const int CONTINUE_BIT = 0b10000000;
	const int SEGMENT_BITS = 0b01111111;
	for (int i = 0; i < 5; ++i) {
		current_byte = buffer_receive_type(uint8_t);
		result += (current_byte & SEGMENT_BITS) << (8 * i - i);
		if ((current_byte & CONTINUE_BIT) != (CONTINUE_BIT)) {
			break;
		}
	}
	return result;
}

uint32_t varint_decode(unsigned char *buffer) {
	unsigned char current_byte = 0;
	int result = 0;
	const int CONTINUE_BIT = 0b10000000;
	const int SEGMENT_BITS = 0b01111111;
	for (int i = 0; i < 5; ++i) {
		current_byte = *(buffer + i);
		result += (current_byte & SEGMENT_BITS) << (8 * i - i);
		if ((current_byte & CONTINUE_BIT) != (CONTINUE_BIT)) {
			break;
		}
	}
	return result;
}

uint8_t *get_bytes(MCVarInt *varInt) {
	return varInt->bytes;
}

unsigned char get_length(MCVarInt *varInt) {
	return varInt->length;
}



