//
// Created by Kasimir on 04.11.2022.
//

#ifndef CMC_OUTGOINGPACKET_H
#define CMC_OUTGOINGPACKET_H


#include "MCVarInt.h"
#include "NetworkBuffer.h"

typedef enum Types {
    PKT_UINT8, PKT_UINT16, PKT_UINT32, PKT_UINT64, PKT_FLOAT,
    PKT_DOUBLE, PKT_STRING, PKT_CHAT, PKT_IDENTIFIER, PKT_VARINT, PKT_VARLONG,
    PKT_ENTITYMETA, PKT_SLOT, PKT_NBTTAG, PKT_OPTIONAL, PKT_ARRAY, PKT_ENUM,
    PKT_BYTEARRAY
} Types;

typedef struct {
    uint8_t members;
    Types *member_types;
    uint8_t type;
    MCVarInt *packet_id;
} PacketWrapper;

void send_pkt_wrapper(PacketWrapper *packet);
void packet_free(PacketWrapper *packet);

typedef struct HandshakePacket {
    PacketWrapper _wrapper;
    MCVarInt *protocol_version;
    MCVarInt *address_length;
    char *ip_address;
    unsigned short port;
    MCVarInt *next_state;
} HandshakePacket;
HandshakePacket* handshake_pkt_new();

struct StatusPacket {
    MCVarInt *packetID;
};

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


