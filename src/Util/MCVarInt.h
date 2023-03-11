#ifndef CMC_MCVARINT_H
#define CMC_MCVARINT_H

#include <stdint.h>
#include "SocketWrapper.h"
#include "NetworkBuffer.h"

typedef struct MCVarInt {
	char size;
	uint8_t bytes[5];
} __attribute__((__packed__)) MCVarInt;

MCVarInt *varint_encode(int givenInt);

int varint_receive(SocketWrapper *socket);

int32_t varint_decode(unsigned char *buffer);

#endif //CMC_MCVARINT_H
