#ifndef CMC_PACKETHANDLER_H
#define CMC_PACKETHANDLER_H

#include "ConnectionState/ConnectionState.h"
#include "SocketWrapper.h"
#include "../State/ClientState.h"

typedef enum Packets {
	// Handshake
	HANDSHAKE_PKT,
	// Status
	STATUS_REQUEST_PKT,
	STATUS_RESPONSE_PKT,
	PING_REQUEST_PKT,
	PING_RESPONSE_PKT,
	// Login
	LOGIN_START_PKT,
	LOGIN_DISCONNECT_PKT,
	ENCRYPTION_REQUEST_PKT,
	ENCRYPTION_RESPONSE_PKT,
	SET_COMPRESSION_PKT,
	LOGIN_SUCCESS_PKT,
	// Play
	LOGIN_PLAY_PKT,
	CHANGE_DIFFICULTY_PKT,
	PLAYER_ABILITIES_CB_PKT,
	DISCONNECT_PLAY_PKT,
	SYNCHRONIZE_PLAYER_POS_PKT,
	CLIENT_INFO_PKT,
	CONFIRM_TELEPORT_PKT,
	SET_PLAYER_POS_PKT,
	SET_PLAYER_POS_ROT_PKT,
    KEEP_ALIVE_CLIENTBOUND_PKT,
    KEEP_ALIVE_SERVERBOUND_PKT,
	SET_PLAYER_ROT_PKT
} Packets;

void handle_packets(ClientState *clientState);

int register_handler(void (*handle)(void *packet), Packets packet_type);

void deregister_all_handlers();

#endif //CMC_PACKETHANDLER_H
