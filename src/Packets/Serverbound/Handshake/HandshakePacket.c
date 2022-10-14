//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdio.h>
#include "HandshakePacket.h"
#include "../../../Util/VarInt/MCVarInt.h"


const int MC_PROTOCOL_VERSION = 760;
const int NEXT_STATE_STATUS = 1;
const int NEXT_STATE_LOGIN = 2;

struct HandshakePacket {
    char packet_id;
    MCVarInt* protocol_version;
    MCVarInt* address_length;
    char* ip_address;
    unsigned short port;
    MCVarInt* next_state;
};

HandshakePacket* header_new(char* ip_address, unsigned char ip_length, unsigned short port) {
    HandshakePacket* header = malloc(sizeof(*header));
    MCVarInt* protocol_version = writeVarInt(MC_PROTOCOL_VERSION);
    MCVarInt* next_state = writeVarInt(NEXT_STATE_STATUS);
    MCVarInt* var_ip_length = writeVarInt(ip_length);

    header->packet_id = 0x00;
    header->protocol_version = protocol_version;
    header->address_length = var_ip_length;
    header->ip_address = ip_address;
    header->port = port;
    header->next_state = next_state;
    return header;
}

int get_header_size(HandshakePacket* header) {
    int size = 0;
    size += sizeof(header->packet_id);
    size += 9;
    size += get_length(header->address_length);
    size += get_length(header->next_state);
    size += get_length(header->protocol_version);
    size += sizeof(header->port);
    return size;
}

NetworkBuffer* get_ptr_buffer(HandshakePacket* header) {
    NetworkBuffer* buffer = buffer_new();
    buffer_write(buffer, &header->packet_id, sizeof(header->packet_id));
    buffer_write_little_endian(buffer, get_bytes(header->protocol_version), get_length(header->protocol_version));
    buffer_write_little_endian(buffer, get_bytes(header->address_length), get_length(header->address_length));
    buffer_write(buffer, header->ip_address, 9);
    buffer_write(buffer, &header->port, sizeof(header->port));
    buffer_write_little_endian(buffer, get_bytes(header->next_state), get_length(header->next_state));
    return buffer;
}