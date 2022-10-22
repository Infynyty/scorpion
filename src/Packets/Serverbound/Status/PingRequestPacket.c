//
// Created by Kasimir on 14.10.2022.
//

#include "PingRequestPacket.h"
#include <stdlib.h>

struct PingRequestPacket {
	MCVarInt *packetID;
	uint64_t *payload;
};

PingRequestPacket *ping_request_packet_new() {
	MCVarInt *packetID = writeVarInt(0x01);
	uint64_t *payload = malloc(sizeof(uint64_t));
	*payload = 1;

	PingRequestPacket *packet = malloc(sizeof(PingRequestPacket));
	packet->packetID = packetID;
	packet->payload = payload;
}

NetworkBuffer *ping_request_packet_write_to_buffer(PingRequestPacket *packet) {
	NetworkBuffer *buffer = buffer_new();
	buffer_write_little_endian(buffer, packet->packetID->bytes, packet->packetID->length);
	buffer_write(buffer, packet->payload, sizeof(uint64_t));
}

int ping_request_packet_send(PingRequestPacket *packet, SocketWrapper *socket) {
	NetworkBuffer *buffer = ping_request_packet_write_to_buffer(packet);
	buffer_send_packet(buffer, socket);
}

void ping_request_packet_free(PingRequestPacket *packet) {
	free(packet->packetID);
	free(packet);
}
