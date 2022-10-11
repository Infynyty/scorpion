//
// Created by Kasimir Stadie on 07.10.22.
//

#ifndef CMC_PACKET_H
#define CMC_PACKET_H

#include <string.h>
#include <stdlib.h>

typedef struct Header Header;

Header* header_new(const char ip_address[256], unsigned short port);
char* get_ptr_buffer(Header* header);
int get_header_size(Header* header);
void print_header(Header* header);


#endif //CMC_PACKET_H
