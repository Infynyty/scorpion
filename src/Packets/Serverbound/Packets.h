//
// Created by Kasimir on 04.11.2022.
//

#ifndef CMC_PACKETS_H
#define CMC_PACKETS_H


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

void packet_send(PacketHeader *packet, SocketWrapper *socket);

void packet_free(PacketHeader *packet);

/** State: Handshake **/

/** Serverbound **/

typedef enum HandshakeNextState {
	HANDSHAKE_STATUS = 1, HANDSHAKE_LOGIN = 2
} HandshakeNextState;

typedef struct __attribute__((__packed__)) HandshakePacket {
	PacketHeader _header;
	MCVarInt *protocol_version;
	NetworkBuffer *address;
	unsigned short port;
	MCVarInt *next_state;
} HandshakePacket;

HandshakePacket *handshake_pkt_new(NetworkBuffer *address, unsigned short port, HandshakeNextState state);


/** State: Status **/

/** Serverbound **/

typedef struct StatusRequestPacket {
	PacketHeader _header;
} StatusRequestPacket;

StatusRequestPacket *status_request_packet_new();

typedef struct StatusResponsePacket {
	PacketHeader _header;
	NetworkBuffer *response;
} StatusResponsePacket;

StatusResponsePacket *status_response_packet_new(NetworkBuffer *response);

struct PingRequestPacket {
	MCVarInt *packetID;
	uint64_t *payload;
};

/** Clientbound **/



/** State: Login **/

/** Serverbound **/

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

/** Clientbound **/


/** State: Play **/

/** Serverbound **/

/** Clientbound **/

typedef struct DisconnectPlayPacket {
    PacketHeader _header;
    NetworkBuffer *reason;
} DisconnectPlayPacket;

DisconnectPlayPacket *disconnect_play_packet_new(NetworkBuffer *reason);

#endif //CMC_PACKETS_H


