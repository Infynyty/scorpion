//
// Created by Kasimir on 14.10.2022.
//

#ifndef CMC_PINGREQUESTPACKET_H
#define CMC_PINGREQUESTPACKET_H

#include "../../../Util/NetworkBuffer.h"

typedef struct PingRequestPacket PingRequestPacket;

PingRequestPacket *ping_request_packet_new();

int ping_request_packet_send(PingRequestPacket *packet, SocketWrapper *socket);

void ping_request_packet_free(PingRequestPacket *packet);

#endif //CMC_PINGREQUESTPACKET_H
