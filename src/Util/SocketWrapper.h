#ifndef CMC_SOCKETWRAPPER_H
#define CMC_SOCKETWRAPPER_H

#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

typedef struct SocketWrapper SocketWrapper;

void connect_wrapper();

SocketWrapper *get_socket();

void close_wrapper();

int receive_wrapper(SocketWrapper *socket, void *bytes, size_t size);

void send_wrapper(SocketWrapper *socket, void *bytes, size_t length);

#endif //CMC_SOCKETWRAPPER_H
