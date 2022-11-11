//
// Created by Kasimir on 16.10.2022.
//

#include "DisconnectLoginPacket.h"
#include "../../../Util/NetworkBuffer.h"


void disconnect_login_packet_handle(SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
    buffer_receive_string(buffer, socket);
	buffer_print_string(buffer);
	buffer_free(buffer);
}