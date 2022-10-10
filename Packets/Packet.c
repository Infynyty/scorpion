//
// Created by Kasimir Stadie on 07.10.22.
//

#include <stdio.h>
#include "Packet.h"
#include "VarInt/VarInt.h"

const int MC_PROTOCOL_VERSION = 760;

struct Header {
    char ip_address[255];
    unsigned short port;
    unsigned char protocol_version[];
};

Header* header_new(const char ip_address[255], unsigned short port) {
    Header* header = malloc(sizeof(*header));
    VarInt* varInt = writeVarInt(MC_PROTOCOL_VERSION);

    memcpy(header->protocol_version, get_bytes(varInt), (int) sizeof(int) * get_length(varInt));
    strcpy(header->ip_address, ip_address);
    header->port = port;
    return header;
}

void print_header(Header* header) {
    printf("\n");
    for (int i = 0; i < 256; i++)
        printf("%d", header->ip_address[i]);
}