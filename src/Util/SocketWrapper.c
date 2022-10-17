//
// Created by Kasimir Stadie on 17.10.22.
//

#ifdef __APPLE__
#include <sys/socket.h>
#include <netinet/in.h>
#include <printf.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "Logger.h"

typedef struct SocketWrapper {
#ifdef _WIN32
    SOCKET socket;
#endif
#ifdef __APPLE__
    int socket;
#endif
} SocketWrapper;

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
    int sock = socket( AF_INET, SOCK_STREAM, 0);
#endif
    socketWrapper->socket = sock;
    struct sockaddr_in server_address;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = htons(25565);
    server_address.sin_family = AF_INET;

    cmc_log(DEBUG, "Trying to connect to port %d at ip %d", server_address.sin_port, INADDR_ANY);
    int connection_status = connect(sock, (const struct sockaddr *) &server_address, sizeof server_address);

    if (connection_status == -1) {
        cmc_log(ERR, "Connection to server could not be established.");
        cmc_log(ERR, "Error code: %d", errno);
        exit(EXIT_FAILURE);
    }
    return socketWrapper;
}

void send_wrapper(SocketWrapper* socket, void* bytes, size_t length) {
#ifdef _WIN32
    send(socket->socket, bytes, length, 0);
#endif
#ifdef __APPLE__
    send(socket->socket, bytes, length, 0);
#endif
}

void receive_wrapper(SocketWrapper *socket, void* bytes, size_t size) {
#ifdef _WIN32
    recv(socket->socket, bytes, size, 0);
#endif
#ifdef __APPLE__
    recv(socket->socket, bytes, size, 0);
#endif
}
