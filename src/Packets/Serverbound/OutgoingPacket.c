//
// Created by Kasimir on 04.11.2022.
//

#include <stdlib.h>
#include "OutgoingPacket.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"
#include "Logger.h"


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
			return sizeof(void *);
	}
}

void packet_send(PacketHeader *packet) {
	NetworkBuffer *buffer = buffer_new();
	void *current_byte = packet + 1;
	buffer_write_little_endian(buffer, packet->packet_id->bytes, packet->packet_id->length);
	for (int i = 0; i < packet->members; ++i) {
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
				MCVarInt *varInt = (*(MCVarInt **) (current_byte));
				buffer_write_little_endian(buffer, varInt->bytes, varInt->length);
				break;
			}
			case PKT_VARLONG:
				//TODO: Fill in
				break;
			case PKT_STRING: {
				NetworkBuffer *string = *((NetworkBuffer **) current_byte);
				MCVarInt *length = writeVarInt(buffer->byte_size);
				buffer_write_little_endian(buffer, length->bytes, length->length);
				buffer_write_little_endian(buffer, string->bytes, string->byte_size);


				free(length);
				break;
			}
			default:
				cmc_log(ERR, "Used unsupported type at packet_send(): %d", m_type);
				exit(EXIT_FAILURE);
		}
		current_byte = (void *) current_byte;
		current_byte += get_types_size(m_type);
	}
}

void packet_free(PacketHeader *packet) {
	void *ptr = packet + 1;
	for (int i = 0; i < packet->members; ++i) {
		Types m_type = packet->member_types[i];
		switch (m_type) {
			case PKT_BOOL:
			case PKT_BYTE:
			case PKT_UINT8:
			case PKT_UINT16:
			case PKT_UINT32:
			case PKT_UINT64:
			case PKT_FLOAT:
			case PKT_DOUBLE:
				ptr += get_types_size(m_type);
				break;
			case PKT_VARINT: {
				MCVarInt **varInt = (MCVarInt **) ptr;
				free(*varInt);
				varInt++;
				ptr = varInt;
				break;
			}
			case PKT_STRING: {
				NetworkBuffer **string = (NetworkBuffer **) ptr;
				free(*string);
				string++;
				ptr = string;
				break;
			}
			case PKT_VARLONG:
			case PKT_IDENTIFIER:
			case PKT_ENTITYMETA:
			case PKT_SLOT:
			case PKT_NBTTAG:
			case PKT_OPTIONAL:
			case PKT_ARRAY:
			case PKT_ENUM:
			case PKT_BYTEARRAY:
			case PKT_CHAT:
			default:
				cmc_log(ERR, "Used unsupported type at packet_free(): %d", m_type);
				exit(EXIT_FAILURE);
		}
	}
	free(packet->member_types);
	free(packet->packet_id);
	free(packet);
}

HandshakePacket *handshake_pkt_new(NetworkBuffer *address, unsigned short port, HandshakeNextState state) {
	HandshakePacket *packet = malloc(sizeof(HandshakePacket));
	uint8_t types_length = 4;
	Types *types = malloc(4 * sizeof(Types));
	Types typeArray[] = {
			PKT_VARINT,
			PKT_STRING,
			PKT_UINT16,
			PKT_VARINT
	};
	memcpy(types, typeArray, types_length * sizeof(Types));
	MCVarInt *packet_id = writeVarInt(0x00);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.direction = SERVERBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->protocol_version = writeVarInt(760);
	packet->address = address;
	packet->port = port;
	packet->next_state = writeVarInt(state);
	return packet;
}

StatusRequestPacket *status_request_packet_new() {
	StatusRequestPacket *packet = malloc(sizeof(StatusRequestPacket));
	uint8_t types_length = 0;
	Types *types = NULL;
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