//
// Created by Kasimir on 15.10.2022.
//

#include <stdbool.h>
#include <stdlib.h>
#include "LoginStartPacket.h"
#include "../../../Util/VarInt/MCVarInt.h"
#include "../../../Util/NetworkBuffer.h"

#define LOGIN_START_PACKET_ID 0x00

struct LoginStartPacket {
    MCVarInt* packet_id;
    MCVarInt* name_length;
    NetworkBuffer* player_name;
    bool has_sig_data;
    bool has_player_uuid;
};

LoginStartPacket* login_start_packet_new() {
    MCVarInt *packet_id = writeVarInt(LOGIN_START_PACKET_ID);
    MCVarInt *name_length = writeVarInt(8);
    NetworkBuffer *player_name = buffer_new();
    char name[] = "Infynyty";
    buffer_write_little_endian(player_name, name, 8);

    LoginStartPacket *packet = malloc(sizeof(LoginStartPacket));
    packet->packet_id = packet_id;
    packet->name_length = name_length;
    packet->player_name = player_name;
    packet->has_sig_data = false;
    packet->has_player_uuid = false;
    return packet;
}

NetworkBuffer* write_login_packet_to_buffer(LoginStartPacket* packet) {
    NetworkBuffer* buffer = buffer_new();
    buffer_write_little_endian(buffer, packet->packet_id->bytes, packet->packet_id->length);
    buffer_write_little_endian(buffer, packet->name_length->bytes, packet->name_length->length);
    buffer_write_little_endian(buffer, packet->player_name->bytes, 8);
    buffer_write(buffer, &packet->has_sig_data, sizeof(packet->has_sig_data));
    buffer_write(buffer, &packet->has_player_uuid, sizeof(packet->has_player_uuid));
    return buffer;
}

int login_start_packet_send(LoginStartPacket* packet, SocketWrapper *socket){
    NetworkBuffer *buffer = write_login_packet_to_buffer(packet);
    buffer_send_packet(buffer, socket);
    buffer_free(buffer);
    return 0;
}
void login_start_packet_free(LoginStartPacket* packet) {
    free(packet->packet_id);
    free(packet->name_length);
    free(packet);
}