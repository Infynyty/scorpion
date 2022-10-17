//
// Created by Kasimir on 14.10.2022.
//

#ifndef CMC_PINGRESPONSEPACKET_H
#define CMC_PINGRESPONSEPACKET_H

#include "SocketWrapper.h"

typedef struct PingResponePacket PingResponsePacket;

void ping_response_packet_handle(SocketWrapper *socket);

#endif //CMC_PINGRESPONSEPACKET_H
