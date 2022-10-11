//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdio.h>
#include "Packet.h"
#include "VarInt/VarInt.h"

const int MC_PROTOCOL_VERSION = 760;
const int NEXT_STATE_STATUS = 1;
const int NEXT_STATE_LOGIN = 2;

struct Header {
    VarInt* protocol_version;
    char ip_address[255];
    unsigned short port;
    VarInt* next_state;
};

Header* header_new(const char ip_address[255], unsigned short port) {
    Header* header = malloc(sizeof(*header));
    VarInt* protocol_version = writeVarInt(MC_PROTOCOL_VERSION);
    VarInt* next_state = writeVarInt(NEXT_STATE_LOGIN);

    header->protocol_version = protocol_version;
    strcpy(header->ip_address, ip_address);
    header->port = port;
    header->next_state = writeVarInt(NEXT_STATE_STATUS);
    return header;
}

void print_header(Header* header) {
    printf("\n");
    for (int i = 0; i < 256; i++)
        printf("%d", header->ip_address[i]);
}

int get_header_size(Header* header) {
    int size = 0;
    size += sizeof(header->ip_address);
    size += get_length(header->next_state);
    size += get_length(header->protocol_version);
    size += sizeof(header->port);
    return size;
}

char* get_ptr_buffer(Header* header) {
    char* buffer = malloc(get_header_size(header));
    int index = 0;
    memcpy(buffer, get_bytes(header->protocol_version), get_length(header->protocol_version));
    index += get_length(header->protocol_version);
    memcpy(buffer + index, header->ip_address, sizeof(header->ip_address));
    index += sizeof(header->ip_address);
    memcpy(buffer + index, &header->port, sizeof(header->port));
    index += sizeof(header->port);
    memcpy(buffer + index, get_bytes(header->next_state), get_length(header->next_state));
    return buffer;
}