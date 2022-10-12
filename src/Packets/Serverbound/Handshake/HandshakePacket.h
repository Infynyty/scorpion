//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_HANDSHAKEPACKET_H
#define CMC_HANDSHAKEPACKET_H

#include <string.h>
#include <stdlib.h>
#include "../../../Util/Buffer.h"

typedef struct HandshakePacket HandshakePacket;

HandshakePacket* header_new(char* ip_address, unsigned char ip_length, unsigned short port);
Buffer* get_ptr_buffer(HandshakePacket* header);
int get_header_size(HandshakePacket* header);
void print_header(HandshakePacket* header);


#endif //CMC_HANDSHAKEPACKET_H
