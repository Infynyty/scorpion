//
// Created by Kasimir on 14.10.2022.
//

#include <stdlib.h>
#include "StatusPacket.h"
#include "../../../Util/VarInt/MCVarInt.h"
#include "../../../Util/NetworkBuffer.h"

//struct StatusRequestPacket {
//	MCVarInt *packetID;
//};
//
//StatusRequestPacket *status_request_packet_new() {
//	StatusRequestPacket *packet = malloc(sizeof(StatusRequestPacket));
//	MCVarInt *packetID = writeVarInt(0);
//	packet->packetID = packetID;
//	return packet;
//}
//
//NetworkBuffer *status_packet_write_to_buffer(StatusRequestPacket *packet) {
//	NetworkBuffer *buffer = buffer_new();
//	buffer_write_little_endian(buffer, packet->packetID->bytes, packet->packetID->length);
//}
//
//void status_packet_send(StatusRequestPacket *packet, SocketWrapper *socket) {
//	NetworkBuffer *buffer = status_packet_write_to_buffer(packet);
//	return buffer_send_packet(buffer, socket);
//}
//
//void status_packet_free(StatusRequestPacket *packet) {
//	free(packet->packetID);
//	free(packet);
//}
