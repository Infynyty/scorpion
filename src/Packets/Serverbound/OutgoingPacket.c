//
// Created by Kasimir on 04.11.2022.
//

#include <stdlib.h>
#include "OutgoingPacket.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"


uint8_t get_types_size(Types type) {
	switch (type) {
		case PKT_BOOL:
			return sizeof(uint8_t);
		case PKT_BYTE:
			return sizeof(int8_t);
		case PKT_UINT8:
			return sizeof(uint8_t);
		case PKT_UINT16:
			return sizeof(uint16_t);
		case PKT_UINT32:
			return sizeof(uint32_t);
		case PKT_UINT64:
			return sizeof(uint64_t);
		case PKT_FLOAT:
			return sizeof(float);
		case PKT_DOUBLE:
			return sizeof(double);
		default:
			return 1;
	}
}

void send_pkt_wrapper(PacketHeader *packet) {
	NetworkBuffer *buffer = buffer_new();

	for (int i = 0; i < packet->members; ++i) {
		void *current_byte = packet++;
		Types m_type = packet->member_types[i];
		switch (m_type) {
			case PKT_BOOL:
			case PKT_UINT8:
			case PKT_UINT16:
			case PKT_UINT32:
			case PKT_UINT64:
			case PKT_FLOAT:
			case PKT_DOUBLE:
				buffer_write(buffer, current_byte, get_types_size(m_type));
				break;
			case PKT_VARINT: {
				MCVarInt *varInt = (MCVarInt *) &current_byte;
				buffer_write_little_endian(buffer, varInt->bytes, varInt->length);
				break;
			}
			case PKT_VARLONG:
				//TODO: Fill in
				break;
			case PKT_STRING: {
				NetworkBuffer *string = (NetworkBuffer *) &current_byte;
				MCVarInt *length = writeVarInt(buffer->byte_size);
				buffer_write_little_endian(buffer, length->bytes, length->length);
				buffer_write_little_endian(buffer, string->bytes, string->byte_size);
			}
			default:
				break;
		}
		current_byte += get_types_size(m_type);
	}
	free(packet->member_types);
	free(packet->packet_id);
	free(packet);
}

void packet_free(PacketHeader *packet) {
	void *ptr = packet++;
	for (int i = 0; i < packet->members; ++i) {
		Types m_type = packet->member_types[i];
		switch (m_type) {
			case PKT_VARINT:
			case PKT_VARLONG:
			case PKT_STRING:
			case PKT_CHAT:
				free(&ptr); //TODO: Check!
			default:
				ptr += get_types_size(m_type);
		}
	}
}

StatusPacket *status_packet_new() {
	StatusPacket *packet = malloc(sizeof(StatusPacket));
	uint8_t types_length = 1;
	Types types[] = {PKT_VARINT};
	MCVarInt *packet_id = writeVarInt(0);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.direction = SERVERBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	return packet;
}