#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "Packets/Serverbound/Handshake/HandshakePacket.h"
#include "Packets/Serverbound/Status/StatusPacket.h"
#include "Packets/Serverbound/Status/PingRequestPacket.h"
#include "Util/ConnectionState/ConnectionState.h"
#include "Packets/Clientbound/PacketHandler.h"
#include "Packets/Serverbound/Login/LoginStartPacket.h"
#include "Util/Logging/Logger.h"
#include "SocketWrapper.h"


#ifdef _WIN32
    #include <winsock2.h>

#endif

#ifdef __APPLE__
    #include <netinet/in.h>
#endif


int main() {

    SocketWrapper* socket_wrapper = connect_wrapper();
    cmc_log(INFO, "Connected succesfully!\n");

    char address[] = "localhost";
    HandshakePacket* handshakePacket = handshake_packet_new(
            address,
            strlen(address),
            25565,
            HANDSHAKE_NEXT_STATE_LOGIN
            );
    handshake_packet_send(handshakePacket, socket_wrapper);
    handshake_packet_free(handshakePacket);

//    StatusPacket* statusPacket = status_packet_new();
//    status_packet_send(statusPacket, sockD);
//    status_packet_free(statusPacket);
//
//
//
//    NetworkBuffer* answer = buffer_new();
//    int packet_length = varint_receive(sockD);
//    printf("Packet length: %d\n", packet_length);
//    int packet_id = varint_receive(sockD);
//    printf("Packet id: %d\n", packet_id);
//    buffer_read_string(answer, sockD);
//    buffer_print_string(answer);
//    buffer_free(answer);
//
//    PingRequestPacket *pingRequestPacket = ping_request_packet_new();
//    ping_request_packet_send(pingRequestPacket, sockD);
//    ping_request_packet_free(pingRequestPacket);

//    handle_incoming_packet(sockD, STATUS);

    LoginStartPacket *loginStartPacket = login_start_packet_new();
    login_start_packet_send(loginStartPacket, socket_wrapper);
    login_start_packet_free(loginStartPacket);

    while (1) {
        handle_incoming_packet(socket_wrapper);
    }


    close(socket_wrapper->socket);

    return 0;
}
