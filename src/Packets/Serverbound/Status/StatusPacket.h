//
// Created by Kasimir on 14.10.2022.
//

#ifndef CMC_STATUSPACKET_H
#define CMC_STATUSPACKET_H

#include <winsock2.h>


typedef struct StatusPacket StatusPacket;

StatusPacket* status_packet_new();
int status_packet_send(StatusPacket* packet, SOCKET socket);
void status_packet_free(StatusPacket* packet);

#endif //CMC_STATUSPACKET_H
