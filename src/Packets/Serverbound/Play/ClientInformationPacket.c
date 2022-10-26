//
// Created by Kasimir on 19.10.2022.
//

#include <malloc.h>
#include "ClientInformationPacket.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"
#include "Logger.h"


void client_information_packet_send(SocketWrapper *socket) {
    MCVarInt *packet_id = writeVarInt(0x08);
    char locale[] = "de_DE";
    char view_distance = 2;
    MCVarInt *chat_mode = writeVarInt(0);
    char is_chat_colored = 1;
    unsigned char display_skin_parts = 0;
    MCVarInt *main_hand = writeVarInt(1);
    char is_text_filter_enabled = 0;
    char show_player_on_serverlist = 1;

    NetworkBuffer *buffer = buffer_new();
    buffer_write_little_endian(buffer, packet_id->bytes, packet_id->length);
    buffer_write_little_endian(buffer, locale, 5);
    buffer_write(buffer, &view_distance, 1);
    buffer_write_little_endian(buffer, chat_mode->bytes, chat_mode->length);
    buffer_write(buffer, &is_chat_colored, 1);
    buffer_write(buffer, &display_skin_parts, 1);
    buffer_write_little_endian(buffer, main_hand->bytes, main_hand->length);
    buffer_write(buffer, &is_text_filter_enabled, 1);
    buffer_write(buffer, &show_player_on_serverlist, 1);

    buffer_send_packet(buffer, socket);
    cmc_log(DEBUG, "Player Information Packet sent.");

    free(packet_id);
    free(chat_mode);
    free(main_hand);
    buffer_free(buffer);
}