//
// Created by Kasimir on 16.10.2022.
//

#include <stdbool.h>
#include "LoginSuccessPacket.h"
#include "../../../Util/NetworkBuffer.h"
#include "../../../Util/Logging/Logger.h"

void login_success_packet_handle(SocketWrapper *socket) {
    u_int64_t uuid_high = buffer_receive_uint64(socket);
    u_int64_t uuid_low = buffer_receive_uint64(socket);
    NetworkBuffer *username = buffer_new();
    buffer_read_string(username, socket);
    int number_of_properties = varint_receive(socket);

    cmc_log(INFO, "Client with username %s logged onto the server. ", username->bytes);
    cmc_log(DEBUG, "Number of properties: %d", number_of_properties);
    cmc_log(DEBUG, "UUID %u%u", uuid_high, uuid_low);

    if (number_of_properties == 0) return;

    NetworkBuffer *name = buffer_new();
    buffer_read_string(name, socket);
    NetworkBuffer *value = buffer_new();
    buffer_read_string(value, socket);
    NetworkBuffer *is_signed_buffer = buffer_new();
    buffer_receive(is_signed_buffer, socket, 1);
    bool is_signed = *(is_signed_buffer->bytes);
    if (is_signed) {
        NetworkBuffer *signature = buffer_new();
        buffer_read_string(signature, socket);
        buffer_free(signature);
    }
    cmc_log(DEBUG, "Name: %s, Values: %s", name->bytes, value->bytes);

    buffer_free(username);
    buffer_free(name);
    buffer_free(value);
}