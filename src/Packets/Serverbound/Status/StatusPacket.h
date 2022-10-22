//
// Created by Kasimir on 14.10.2022.
//

#ifndef CMC_STATUSPACKET_H
#define CMC_STATUSPACKET_H


#include "../../../Util/SocketWrapper.h"

typedef struct StatusPacket StatusPacket;

StatusPacket *status_packet_new();

void status_packet_send(StatusPacket *packet, SocketWrapper *socket);

void status_packet_free(StatusPacket *packet);

#endif //CMC_STATUSPACKET_H
