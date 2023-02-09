//
// Created by Kasimir Stadie on 17.10.22.
//

#if defined(__APPLE__) || defined(__linux__)

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
#if defined(__APPLE__) || defined(__linux__)
	int socket;
#endif
} SocketWrapper;

static const SocketWrapper *server;

SocketWrapper *connect_wrapper() {
	SocketWrapper *socketWrapper = malloc(sizeof(SocketWrapper));
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}
	SOCKET sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
#if defined(__APPLE__) || defined(__linux__)
	int sock = socket(AF_INET, SOCK_STREAM, 0);
#endif
	socketWrapper->socket = sock;
	struct sockaddr_in server_address;
	server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	server_address.sin_port = htons(25565);
	server_address.sin_family = AF_INET;

	cmc_log(DEBUG, "Trying to connect to port %d at ip %d", server_address.sin_port, INADDR_LOOPBACK);
	int connection_status = connect(sock, (const struct sockaddr *) &server_address, sizeof server_address);

	if (connection_status == -1) {
		cmc_log(ERR, "Connection to server could not be established.");
		cmc_log(ERR, "Error code: %d", errno);
		exit(EXIT_FAILURE);
	}
	server = socketWrapper;
	return socketWrapper;
}

const SocketWrapper *get_socket() {
	return server;
}

void send_wrapper(SocketWrapper *socket, void *bytes, size_t length) {
#ifdef _WIN32
	send(socket->socket, bytes, length, 0);
#endif
#if defined(__APPLE__) || defined(__linux__)
	send(socket->socket, bytes, length, 0);
#endif
}

int receive_wrapper(SocketWrapper *socket, void *bytes, size_t size) {
#ifdef _WIN32
	int response = recv(socket->socket, bytes, size, 0);
	if (response == -1) {
		cmc_log(ERR, "Error during receive: %d, check Winsock Error codes for more info.", WSAGetLastError());
	}
	return response;
#endif
#if defined(__APPLE__) || defined(__linux__)
	return recv(socket->socket, bytes, size, 0);
#endif
}
