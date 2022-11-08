//
// Created by Kasimir on 04.11.2022.
//

#ifndef CMC_OUTGOINGPACKET_H
#define CMC_OUTGOINGPACKET_H


#include <stdbool.h>
#include "MCVarInt.h"
#include "NetworkBuffer.h"
#include "ConnectionState.h"

typedef enum Types {
	PKT_BOOL, PKT_BYTE, PKT_UINT8, PKT_UINT16, PKT_UINT32, PKT_UINT64, PKT_FLOAT,
	PKT_DOUBLE, PKT_STRING, PKT_CHAT, PKT_IDENTIFIER, PKT_VARINT, PKT_VARLONG,
	PKT_ENTITYMETA, PKT_SLOT, PKT_NBTTAG, PKT_OPTIONAL, PKT_ARRAY, PKT_ENUM,
	PKT_BYTEARRAY
} Types;

typedef struct {
	uint8_t members;
	Types *member_types;
	ConnectionState state;
	PacketDirection direction;
	MCVarInt *packet_id;
} PacketHeader;

void send_pkt_wrapper(PacketHeader *packet);

void packet_free(PacketHeader *packet);

typedef struct HandshakePacket {
	PacketHeader _header;
	MCVarInt *protocol_version;
	MCVarInt *address_length;
	char *ip_address;
	unsigned short port;
	MCVarInt *next_state;
} HandshakePacket;

HandshakePacket *handshake_pkt_new();

typedef struct StatusPacket {
	PacketHeader _header;
} StatusPacket;

StatusPacket *status_packet_new();

struct PingRequestPacket {
	MCVarInt *packetID;
	uint64_t *payload;
};

struct LoginStartPacket {
	MCVarInt *packet_id;
	MCVarInt *name_length;
	NetworkBuffer *player_name;
	bool has_sig_data;
	bool has_player_uuid;
};

struct ConfirmTeleportationPacket {
	MCVarInt *packetID;
	MCVarInt *teleportID;
};


#endif //CMC_OUTGOINGPACKET_H


