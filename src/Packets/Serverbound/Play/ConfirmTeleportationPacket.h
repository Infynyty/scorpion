//
// Created by Kasimir Stadie on 22.10.22.
//

#ifndef CMC_CONFIRMTELEPORTATIONPACKET_H
#define CMC_CONFIRMTELEPORTATIONPACKET_H

#include "SocketWrapper.h"
#include "../../../State/ClientState.h"

typedef struct ConfirmTeleportationPacket ConfirmTeleportationPacket;

ConfirmTeleportationPacket* confirm_teleportation_packet_new(ClientState *client);
void confirm_teleportation_packet_send(ConfirmTeleportationPacket* packet, SocketWrapper *socket);
void confirm_teleportation_packet_free(ConfirmTeleportationPacket* packet);

#endif //CMC_CONFIRMTELEPORTATIONPACKET_H
