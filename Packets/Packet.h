//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_PACKET_H
#define CMC_PACKET_H

#include <string.h>
#include <stdlib.h>
#include "../Buffer.h"

typedef struct Header Header;

Header* header_new(char* ip_address, unsigned char ip_length, unsigned short port);
Buffer* get_ptr_buffer(Header* header);
int get_header_size(Header* header);
void print_header(Header* header);


#endif //CMC_PACKET_H
