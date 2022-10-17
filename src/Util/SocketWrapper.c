//
// Created by Kasimir Stadie on 17.10.22.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "SocketWrapper.h"
#include "NetworkBuffer.h"

struct SocketWrapper {
#ifdef _WIN32
    SOCKET socket;
#endif
#ifdef __APPLE__
    int socket;
#endif
};

SocketWrapper* connect_wrapper() {
    SocketWrapper *socketWrapper = malloc(sizeof(SocketWrapper));
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }
    SOCKET sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
#ifdef __APPLE__
    int sock = socket( PF_LOCAL, SOCK_STREAM, 0);
#endif
    socketWrapper->socket = sock;
    struct sockaddr_in server_address;
    uint32_t server_ip = 2130706433; // 127.0.0.1 as an integer
    server_address.sin_addr.s_addr = server_ip;
    server_address.sin_port = htons(25565);
    server_address.sin_family = AF_INET;

    printf("Trying to connect_wrapper to port %d.\n", server_address.sin_port);
    int connection_status = connect(sock, (const struct sockaddr *) &server_address, sizeof server_address);

    if (connection_status == -1) {
        printf("Fehler!");
    }
    return socketWrapper;
}

void send_wrapper(SocketWrapper* socket) {

}

void receive_wrapper(SocketWrapper *socket, NetworkBuffer *buffer, size_t size) {

}
