//
// Created by Kasimir Stadie on 17.10.22.
//

#ifndef CMC_SOCKETWRAPPER_H
#define CMC_SOCKETWRAPPER_H

#include <stddef.h>
#ifdef _WIN32
#include <winsock2.h>
#endif

typedef struct SocketWrapper {
#ifdef _WIN32
    SOCKET socket;
#endif
#ifdef __APPLE__
    int socket;
#endif
} SocketWrapper;

SocketWrapper* connect_wrapper();

void receive_wrapper(SocketWrapper *socket, void* bytes, size_t size);

void send_wrapper(SocketWrapper* socket, void* bytes, size_t length);

#endif //CMC_SOCKETWRAPPER_H
