//
// Created by Kasimir Stadie on 07.10.22.
//

#include "Packet.h"
#include "VarInt/VarInt.h"

struct Header {
    char ip_address[256];
    unsigned short port;
    unsigned char protocol_version[];
};

Header* header_new(const char ip_address[256], unsigned short port) {
    Header* header = malloc(sizeof(*header));
    VarInt* varInt = writeVarInt(PROTOCOL_VERSION);

    memcpy(header->protocol_version, get_bytes(varInt), sizeof(int * ));
    strcpy(header->ip_address, ip_address);
    header->port = port;
    return header;
}