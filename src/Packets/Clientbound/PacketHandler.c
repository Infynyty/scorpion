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
#include "../Serverbound/Packets.h"


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



typedef void (*HandlePacket)(void *packet);

typedef struct HandlerNode {
	int id;

	void (*handle)(void *packet);

	struct HandlerNode *next;
} HandlerNode;


//TODO: find better method
static HandlerNode *packet_handlers[SET_PLAYER_ROT_PKT + 1];

void deregister_all_handlers() {
    for (int i = 0; i < SET_PLAYER_POS_ROT_PKT + 1; ++i) {
        HandlerNode *current_node = packet_handlers[i];
        while (current_node != NULL) {
            HandlerNode *tmp = current_node->next;
            free(current_node);
            current_node = tmp;
        }
    }
}

int add_node(HandlePacket handle, Packets packet_type) {
	HandlerNode *new_node = malloc(sizeof(HandlerNode));
	new_node->handle = handle;
	new_node->id = 0;
    new_node->next = NULL;

	HandlerNode *current_node = packet_handlers[packet_type];
	if (current_node == NULL) {
		packet_handlers[packet_type] = new_node;
		return 0;
	}
	while (1) {
		if (current_node->next == NULL) {
			new_node->id = current_node->id + 1;
			current_node->next = new_node;
			return new_node->id;
		}
        current_node = current_node->next;
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
	return add_node(handle, packet_type);
}

void packet_event(Packets packet_type, PacketHeader* packet) {
	HandlerNode *current_node = packet_handlers[packet_type];
	while (current_node != NULL) {
        (*current_node->handle)(packet);
        current_node = current_node->next;
	}
    packet_free(packet);
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
                case STATUS_RESPONSE: {
                    StatusResponsePacket *packet = status_response_packet_new(NULL);
                    NetworkBuffer *response = buffer_new();
                    buffer_receive_string(response, socket);
                    packet->response = response;
                    packet_event(STATUS_RESPONSE_PKT, &(packet->_header));
                    break;
                }
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
				case DISCONNECT_PLAY: {
                    DisconnectPlayPacket *packet = disconnect_play_packet_new(NULL);
                    NetworkBuffer *response = buffer_new();
                    buffer_receive_string(response, socket);
                    packet->reason = response;
                    packet_event(DISCONNECT_PLAY_PKT, &(packet->_header));
                    return;
                }
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

void consume_packet(SocketWrapper *socket, int length_in_bytes) {
	char *ptr = malloc(length_in_bytes);
	receive_wrapper(socket, ptr, length_in_bytes);
	free(ptr);
}

