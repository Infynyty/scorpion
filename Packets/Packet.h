//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_PACKET_H
#define CMC_PACKET_H

#include <string.h>
#include <stdlib.h>

const int PROTOCOL_VERSION = 760;

typedef struct Header Header;

Header* header_new(const char ip_address[256], unsigned short port);


#endif //CMC_PACKET_H
