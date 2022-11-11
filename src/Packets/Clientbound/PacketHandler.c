//
// Created by Kasimir on 14.10.2022.
//

#include "PacketHandler.h"
#include "../../Util/VarInt/MCVarInt.h"
#include <stdio.h>
#include <stdlib.h>

#include "Status/PingResponsePacket.h"
#include "../../Util/NetworkBuffer.h"
#include "../../Util/Logging/Logger.h"
#include "Login/SetCompressionPacket.h"
#include "Login/DisconnectLoginPacket.h"
#include "Login/LoginSuccessPacket.h"
#include "Play/SynchronizePlayerPositionPacket.h"


// Connection status: STATUS
#define STATUS_RESPONSE             0x00
#define PING_RESPONSE               0x01

// Connection status: LOGIN

#define DISCONNECT_LOGIN            0x00
#define LOGIN_SUCCESS               0x02
#define SET_COMPRESSION             0x03

// Connection status: PLAY

#define SYNCHRONIZE_PLAYER_POSITION 0x39
#define DISCONNECT_PLAY             0x19
#define LOGIN_PLAY                  0x25

void consume_packet(SocketWrapper *socket, int length_in_bytes);

// TODO: Write event listener system

// Linked List system: ll for every packet type, array of lls? (no of total packets: ?)
// UUID for every packet != packet id

typedef enum Packets {
	// Handshake
	HANDSHAKE_PKT,
	// Status
	STATUS_REQUEST_PKT, STATUS_RESPONSE_PKT, PING_REQUEST_PKT, PING_RESPONSE_PKT,
	// Login
	LOGIN_START,
	// Play
	CLIENT_INFO_PKT, CONFIRM_TELEPORT_PKT, SET_PLAYER_POS_PKT, SET_PLAYER_POS_ROT_PKT, SET_PLAYER_ROT_PKT
} Packets;

typedef void (*HandlePacket)(void *packet);

typedef struct HandlerNode {
	int id;

	void (*handle)(void *packet);

	struct HandlerNode *next;
} HandlerNode;

//TODO: find better method
static HandlerNode *packet_handlers[SET_PLAYER_ROT_PKT + 1];

int add_node(HandlePacket handle, Packets packet_type) {
	HandlerNode *new_node = malloc(sizeof(HandlerNode));
	new_node->handle = handle;
	new_node->id = 0;

	HandlerNode *current_node = packet_handlers[packet_type];
	if (current_node->next == NULL) {
		packet_handlers[packet_type] = new_node;
		return 0;
	}
	while (1) {
		if (current_node->next == NULL) {
			new_node->id = current_node->id + 1;
			current_node->next = new_node;
			return new_node->id;
		}
	}
}

void remove_node(int id, Packets packet_type) {
	HandlerNode *current_node = packet_handlers[packet_type];
	HandlerNode *previous_node = NULL;
	while (current_node->next != NULL) {
		if (current_node->id == id) {
			if (previous_node != NULL) {
				previous_node->next = current_node->next;
			}
			packet_handlers[packet_type] = current_node->next;
			free(current_node->next);
			free(current_node);
			break;
		}
		previous_node = current_node;
		current_node = current_node->next;
	}
}

int register_handler(void (*handle)(void *packet), Packets packet_type) {
	HandlerNode *handler = malloc(sizeof(HandlerNode));
	handler->handle = handle;
	add_node(handle, packet_type);
	return handler->id;
}

void packet_event(Packets packet_type) {
	HandlerNode *current_node = packet_handlers[packet_type];
	while (current_node->next != NULL) {
		current_node->handle
	}
}

void handle_packets(SocketWrapper *socket, ClientState *clientState) {
	Packets packet_type;
	ConnectionState connectionState = STATUS;
	int packet_length_total = varint_receive(socket) - 1; //TODO: Assumes packet ids are always one byte

	if (packet_length_total == -1) {
		return;
	}
	int packet_id = varint_receive(socket);

	if (packet_id > 1000) {
		return;
	}
	switch (connectionState) {
		case STATUS:
			switch (packet_id) {
				case STATUS_RESPONSE:
					break;
				case PING_RESPONSE:
					ping_response_packet_handle(socket);
					break;
				default:
					consume_packet(socket, packet_length_total);
					cmc_log(DEBUG, "Consumed packet with id %d.", packet_id);
			}
			break;


		case LOGIN:
			switch (packet_id) {
				case DISCONNECT_LOGIN:
					disconnect_login_packet_handle(socket);
					return;
				case LOGIN_SUCCESS:
					login_success_packet_handle(socket);
					connectionState = PLAY;
					break;
				case SET_COMPRESSION:
					set_compression_packet_handle(socket);
					break;
				default:
					consume_packet(socket, packet_length_total);
					cmc_log(DEBUG, "Consumed packet with id %d in Login State.", packet_id);
			}
			break;


		case PLAY:
			switch (packet_id) {
				case DISCONNECT_PLAY:
					cmc_log(INFO, "Disconnect play packet received.");
					NetworkBuffer *string = buffer_new();
					buffer_read_string(string, socket);
					buffer_print_string(string);
					return;
				case LOGIN_PLAY:
					cmc_log(INFO, "Login play packet received.");
					break;
				case SYNCHRONIZE_PLAYER_POSITION:
					synchronize_player_position_packet_handle(socket, clientState);
					break;
				default:
					consume_packet(socket, packet_length_total);
					cmc_log(DEBUG, "Consumed packet with id %x in Play State.", packet_id);
			}
			break;


		default:
			cmc_log(ERR, "Invalid connection state (state: %d)!", connectionState);
			return;
	}
}

void handle_incoming_packet(SocketWrapper *socket, ClientState *clientState) {
	enum ConnectionState connectionState = STATUS;
	while (1) {
		int packet_length_total = varint_receive(socket) - 1; //TODO: Assumes packet ids are always one byte

		if (packet_length_total == -1) {
			return;
		}
		int packet_id = varint_receive(socket);

		if (packet_id > 1000) {
			return;
		}

		cmc_log(DEBUG, "Packet received with ID: 0x%x and size %d", packet_id, packet_length_total);
		switch (connectionState) {
			case STATUS:
				switch (packet_id) {
					case STATUS_RESPONSE:
						break;
					case PING_RESPONSE:
						ping_response_packet_handle(socket);
						break;
					default:
						consume_packet(socket, packet_length_total);
						cmc_log(DEBUG, "Consumed packet with id %d.", packet_id);
				}
				break;


			case LOGIN:
				switch (packet_id) {
					case DISCONNECT_LOGIN:
						disconnect_login_packet_handle(socket);
						return;
					case LOGIN_SUCCESS:
						login_success_packet_handle(socket);
						connectionState = PLAY;
						break;
					case SET_COMPRESSION:
						set_compression_packet_handle(socket);
						break;
					default:
						consume_packet(socket, packet_length_total);
						cmc_log(DEBUG, "Consumed packet with id %d in Login State.", packet_id);
				}
				break;


			case PLAY:
				switch (packet_id) {
					case DISCONNECT_PLAY:
						cmc_log(INFO, "Disconnect play packet received.");
						NetworkBuffer *string = buffer_new();
						buffer_read_string(string, socket);
						buffer_print_string(string);
						return;
					case LOGIN_PLAY:
						cmc_log(INFO, "Login play packet received.");
						break;
					case SYNCHRONIZE_PLAYER_POSITION:
						synchronize_player_position_packet_handle(socket, clientState);
						break;
					default:
						consume_packet(socket, packet_length_total);
						cmc_log(DEBUG, "Consumed packet with id %x in Play State.", packet_id);
				}
				break;


			default:
				cmc_log(ERR, "Invalid connection state (state: %d)!", connectionState);
				return;
		}
	}
}

void consume_packet(SocketWrapper *socket, int length_in_bytes) {
	char *ptr = malloc(length_in_bytes);
	receive_wrapper(socket, ptr, length_in_bytes);
	free(ptr);
}

