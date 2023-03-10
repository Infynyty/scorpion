#ifndef CMC_PACKETHANDLER_H
#define CMC_PACKETHANDLER_H

#include "SocketWrapper.h"
#include "../State/ClientState.h"
#include "PlayState.h"

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
	PLAYER_CHAT_MESSAGE_PKT,
    KEEP_ALIVE_CLIENTBOUND_PKT,
    CHUNK_DATA_PKT,
    UNLOAD_CHUNK_PKT,
    KEEP_ALIVE_SERVERBOUND_PKT,
	SET_PLAYER_ROT_PKT,

    //Always last, to count the number of defined packets
    PACKET_COUNT
} Packets;

typedef struct PacketHandleWrapper {
    PlayState *state;
    GenericPacketList *list;
    pthread_t *receive_thread;
} PacketHandleWrapper;

/**
 * Consumes packets that are added to a shared linked list by the function <packet_receive>. This function is thread-safe.
 * Consuming packets includes generating the right struct for any given generic packet and then calling triggering a
 * packet event for the correct packet type. The packet is freed afterwards.
 *
 * @param wrapper Contains the current play state, the list to consume the packets from and a thread reference to start
 *                receiving packets concurrently after the login has been completed.
 * @return        NULL
 */
void *handle_packets(void *wrapper);


/**
 * Registered a packet handler. A packet handler will be called when the correct packet is received by <handle_packets>.
 *
 * @param handle        A pointer to the handler method. Any handler method can access the received packet by casting
 *                      the packet pointer to the right packet.
 * @param packet_type   The type of packet this handler should be called for.
 * @return              The handler id.
 */
int register_handler(void (*handle)(void *packet, PlayState *state), Packets packet_type);

/**
 * Deregisters all currently registered handlers and frees associated pointers.
 */
void deregister_all_handlers();

#endif //CMC_PACKETHANDLER_H
