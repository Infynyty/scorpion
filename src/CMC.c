#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>


#ifdef _WIN32
    #include <winsock2.h>
#include "Packets/Serverbound/Handshake/HandshakePacket.h"
#include "Packets/Serverbound/Status/StatusPacket.h"
#include "Packets/Serverbound/Status/PingRequestPacket.h"
#include "Util/ConnectionState/ConnectionState.h"
#include "Packets/Clientbound/PacketHandler.h"

#endif

#ifdef __APPLE__
    #include <netinet/in.h>
#endif


int main() {


#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }
    SOCKET sockD = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
#ifdef __APPLE__
    int sockD = socket( PF_LOCAL, SOCK_STREAM, 0);
#endif

    struct sockaddr_in server_address;
//    uint32_t ip_address = 2130706433; // 127.0.0.1 as an integer
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(25565);
    server_address.sin_family = AF_INET;

    printf("Trying to connect to port %d.\n", server_address.sin_port);
    int connection_status = connect(sockD, (const struct sockaddr *) &server_address, sizeof server_address);

    if (connection_status == -1) {
        printf("Fehler!");
        return -1;
    }
    printf("Connected succesfully!\n");

    char address[] = "localhost";
    HandshakePacket* handshakePacket = handshake_packet_new(
            address,
            strlen(address),
            25565,
            HANDSHAKE_NEXT_STATE_STATUS
            );
    handshake_packet_send(handshakePacket, sockD);
    handshake_packet_free(handshakePacket);

    StatusPacket* statusPacket = status_packet_new();
    status_packet_send(statusPacket, sockD);
    status_packet_free(statusPacket);



    NetworkBuffer* answer = buffer_new();
    int packet_length = varint_receive(sockD);
    printf("Packet length: %d\n", packet_length);
    int packet_id = varint_receive(sockD);
    printf("Packet id: %d\n", packet_id);
    buffer_read_string(answer, sockD);
    buffer_print_string(answer);
    buffer_free(answer);

    PingRequestPacket *pingRequestPacket = ping_request_packet_new();
    ping_request_packet_send(pingRequestPacket, sockD);
    ping_request_packet_free(pingRequestPacket);

    handle_incoming_packet(sockD, STATUS);

    close(connection_status);

    return 0;
}
