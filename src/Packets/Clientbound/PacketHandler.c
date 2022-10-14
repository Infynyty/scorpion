//
// Created by Kasimir on 14.10.2022.
//

#include "PacketHandler.h"
#include "../../Util/VarInt/MCVarInt.h"
#include <stdio.h>

#include "Status/PingResponsePacket.h"
#include "../../Util/NetworkBuffer.h"


// Connection status: STATUS
#define STATUS_RESPONSE     0x00
#define PING_RESPONSE       0x01

// Connection status: LOGIN

// Connection status: PLAY

#define DISCONNECT_PLAY     0x19
#define LOGIN_PLAY          0x25


void handle_incoming_packet(SOCKET socket, enum ConnectionState connectionState) {
    int packet_length = varint_receive(socket);
    int packet_id = varint_receive(socket);

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
                    fprintf(stderr, "Invalid packet id (packet id: %d)!", packet_id);
                    exit(EXIT_FAILURE);
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
            fprintf(stderr, "Invalid connection state (state: %d)!", connectionState);
            exit(EXIT_FAILURE);
    }
}

