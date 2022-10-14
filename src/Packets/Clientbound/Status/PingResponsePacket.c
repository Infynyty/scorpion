//
// Created by Kasimir on 14.10.2022.
//

#include <stdio.h>
#include "PingResponsePacket.h"
#include "../../../Util/NetworkBuffer.h"

void ping_response_packet_handle(SOCKET socket) {
    NetworkBuffer* buffer = buffer_new();
    buffer_receive(buffer, socket, sizeof(uint64_t));

    buffer_free(buffer);
    printf("Ping response: ");
}
