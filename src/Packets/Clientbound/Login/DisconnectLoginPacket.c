//
// Created by Kasimir on 16.10.2022.
//

#include "DisconnectLoginPacket.h"
#include "../../../Util/NetworkBuffer.h"


void disconnect_login_packet_handle(SOCKET socket) {
    NetworkBuffer *buffer = buffer_new();
    buffer_read_string(buffer, socket);
    buffer_print_string(buffer);
    buffer_free(buffer);
}