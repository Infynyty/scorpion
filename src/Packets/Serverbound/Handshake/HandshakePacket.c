//
// Created by Kasimir Stadie on 07.10.22.
//

#include "HandshakePacket.h"


const int MC_PROTOCOL_VERSION = 760;

struct HandshakePacket {
    char packet_id;
    MCVarInt *protocol_version;
    MCVarInt *address_length;
    char *ip_address;
    unsigned short port;
    MCVarInt *next_state;
};

HandshakePacket *handshake_packet_new(
        char *ip_address,
        unsigned char ip_length,
        unsigned short port,
        const int next_state
        ) {
    HandshakePacket *header = malloc(sizeof(*header));
    MCVarInt *protocol_version = writeVarInt(MC_PROTOCOL_VERSION);
    MCVarInt *next_state_varint = writeVarInt(next_state);
    MCVarInt *var_ip_length = writeVarInt(ip_length);

    header->packet_id = 0x00;
    header->protocol_version = protocol_version;
    header->address_length = var_ip_length;
    header->ip_address = ip_address;
    header->port = port;
    header->next_state = next_state_varint;
    return header;
}

NetworkBuffer *write_packet_to_buffer(HandshakePacket *packet) {
    NetworkBuffer *buffer = buffer_new();
    buffer_write(buffer, &packet->packet_id, sizeof(packet->packet_id));
    buffer_write_little_endian(buffer, get_bytes(packet->protocol_version), get_length(packet->protocol_version));
    buffer_write_little_endian(buffer, get_bytes(packet->address_length), get_length(packet->address_length));
    buffer_write(buffer, packet->ip_address, 9);
    buffer_write(buffer, &packet->port, sizeof(packet->port));
    buffer_write_little_endian(buffer, get_bytes(packet->next_state), get_length(packet->next_state));
    return buffer;
}

void handshake_packet_send(HandshakePacket *packet, SocketWrapper *socket) {
    return buffer_send_packet(write_packet_to_buffer(packet), socket);
}

void handshake_packet_free(HandshakePacket* packet) {
    free(packet->protocol_version);
    free(packet->address_length);
    free(packet->next_state);
    free(packet);
}