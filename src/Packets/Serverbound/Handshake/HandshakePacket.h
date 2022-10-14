//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_HANDSHAKEPACKET_H
#define CMC_HANDSHAKEPACKET_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../../../Util/NetworkBuffer.h"

#define HANDSHAKE_NEXT_STATE_STATUS 1
#define HANDSHAKE_NEXT_STATE_LOGIN 2

typedef struct HandshakePacket HandshakePacket;

HandshakePacket* handshake_packet_new(char* ip_address, unsigned char ip_length, unsigned short port, int next_state);
int handshake_packet_send(HandshakePacket* packet, SOCKET socket);
void handshake_packet_free(HandshakePacket* packet);


#endif //CMC_HANDSHAKEPACKET_H
