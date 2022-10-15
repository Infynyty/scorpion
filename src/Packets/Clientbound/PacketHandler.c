//
// Created by Kasimir on 14.10.2022.
//

#include "PacketHandler.h"
#include "../../Util/VarInt/MCVarInt.h"
#include <stdio.h>

#include "Status/PingResponsePacket.h"
#include "../../Util/NetworkBuffer.h"
#include "../../Util/Logging/Logger.h"


// Connection status: STATUS
#define STATUS_RESPONSE     0x00
#define PING_RESPONSE       0x01

// Connection status: LOGIN

// Connection status: PLAY

#define DISCONNECT_PLAY     0x19
#define LOGIN_PLAY          0x25

void consume_packet(SOCKET socket, int length_in_bytes);

void handle_incoming_packet(SOCKET socket, enum ConnectionState connectionState) {
    int packet_length = varint_receive(socket);
    int packet_id = varint_receive(socket);

    if(packet_id > 1000) {
        return;
    }

    printf("Packet received with ID: %d\n", packet_id);
    switch (connectionState) {
        case HANDSHAKE:
            break;


        case STATUS:
            switch (packet_id) {
                case STATUS_RESPONSE:
                    break;
                case PING_RESPONSE:
                    ping_response_packet_handle(socket);
                    break;
                default:
                    consume_packet(socket, packet_length - 1);
                    cmc_log(DEBUG, "Consumed packet with id %x", packet_id);
            }
            break;


        case LOGIN:
            break;


        case PLAY:
            switch (packet_id) {
                case DISCONNECT_PLAY:
                    printf("Disconnect play packet received.");
                    NetworkBuffer *string = buffer_new();
                    buffer_read_string(string, socket);
                    buffer_print_string(string);
                    break;
                case LOGIN_PLAY:
                    printf("Login play packet received.");
                    break;
            }
            break;


        default:
            cmc_log(ERR, "Invalid connection state (state: %d)!", connectionState);
            exit(EXIT_FAILURE);
    }
}

void consume_packet(SOCKET socket, int length_in_bytes) {
    char* ptr = malloc(length_in_bytes);
    recv(socket, ptr, length_in_bytes, 0);
    free(ptr);
}

