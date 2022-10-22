//
// Created by Kasimir Stadie on 22.10.22.
//

#include <stdlib.h>
#include "ConfirmTeleportationPacket.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"

#define CONFIRM_TELEPORTATION_PACKET 0x00

struct ConfirmTeleportationPacket {
	MCVarInt *packetID;
	MCVarInt *teleportID;
};

ConfirmTeleportationPacket *confirm_teleportation_packet_new(ClientState *client) {
	MCVarInt *packetID = writeVarInt(CONFIRM_TELEPORTATION_PACKET);
	MCVarInt *teleportID = writeVarInt(client_get_teleportID(client));

	ConfirmTeleportationPacket *packet = malloc(sizeof(ConfirmTeleportationPacket));
	packet->packetID = packetID;
	packet->teleportID = teleportID;

	free(packetID);
	free(teleportID);
	return packet;
}

void confirm_teleportation_packet_send(ConfirmTeleportationPacket *packet, SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	buffer_write(buffer, packet->packetID->bytes, packet->packetID->length);
	buffer_write(buffer, packet->teleportID->bytes, packet->teleportID->length);
	buffer_free(buffer);
}

void confirm_teleportation_packet_free(ConfirmTeleportationPacket *packet) {
	free(packet->packetID);
	free(packet->teleportID);
	free(packet);
}