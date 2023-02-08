#include "PacketHandler.h"
#include "VarInt/MCVarInt.h"
#include <stdlib.h>
#include <unistd.h>

#include "NetworkBuffer.h"
#include "Logging/Logger.h"
#include "Packets.h"
#include "NBTParser.h"
#include "ServerState.h"
#include "Encryption.h"


// Connection status: STATUS
#define STATUS_RESPONSE                 0x00
#define PING_RESPONSE                   0x01

// Connection status: LOGIN

#define DISCONNECT_LOGIN                0x00
#define ENCRYPTION_REQUEST_ID           0x01
#define LOGIN_SUCCESS_ID                0x02
#define SET_COMPRESSION_ID              0x03

// Connection status: PLAY

#define CHANGE_DIFFICULTY_ID            0x0b
#define PLAYER_ABILITIES_CB_ID          0x31
#define SYNCHRONIZE_PLAYER_POSITION_ID  0x39
#define DISCONNECT_PLAY                 0x19
#define LOGIN_PLAY_ID                   0x25
#define SET_HELD_ITEM_ID                0x4a
#define UPDATE_RECIPES_ID               0x6a

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

void packet_event(Packets packet_type, PacketHeader *packet) {
	HandlerNode *current_node = packet_handlers[packet_type];
	while (current_node != NULL) {
		(*current_node->handle)(packet);
		current_node = current_node->next;
	}
	packet_free(packet);
}

static ServerState *serverState;


//TODO: abstract method for packet receive using packet fields?
void handle_packets(SocketWrapper *socket, ClientState *clientState) {
    serverState = serverstate_new();
	Packets packet_type;
	ConnectionState connectionState = STATUS;

	while (true) {
		GenericPacket *generic_packet = packet_receive();

		if (generic_packet->uncompressed_length < 0) {
			continue;
		}

		if (generic_packet->packet_id > 255) {
			cmc_log(ERR, "Illegal packet id %d, packet size: %d", generic_packet->packet_id,
			        generic_packet->uncompressed_length);
			continue;
		}
		switch (connectionState) {
			case STATUS:
				switch (generic_packet->packet_id) {
					case STATUS_RESPONSE: {
						StatusResponsePacket *packet = status_response_packet_new(NULL);
						NetworkBuffer *response = buffer_new();
						buffer_receive_string(response, socket);
						packet->response = response;
						packet_event(STATUS_RESPONSE_PKT, &(packet->_header));
						break;
					}
					default:
						cmc_log(DEBUG, "Consumed packet with id %d.", generic_packet->packet_id);
				}
				break;


			case LOGIN:
				switch (generic_packet->packet_id) {
					case ENCRYPTION_REQUEST_ID: {
						cmc_log(INFO, "Received encryption request.");
						EncryptionRequestPacket *packet = encryption_request_packet_new(NULL, NULL, NULL);
						packet_decode(&packet->_header, generic_packet->data);

                        serverState->public_key = packet->public_key;
                        serverState->verify_token = packet->verify_token;

                        NetworkBuffer *secret = buffer_new();
                        EncryptionResponsePacket *response = encryption_response_generate(
                                packet->public_key,
                                packet->verify_token,
                                secret
                                );
                        packet_send(&response->_header);
                        init_encryption(secret);
                        buffer_free(secret);

						packet_event(ENCRYPTION_REQUEST_PKT, &packet->_header);
						break;
					}
					case LOGIN_SUCCESS_ID: {
						cmc_log(DEBUG, "Received Login Success Packet.");
						connectionState = PLAY;
						cmc_log(DEBUG, "Switched connection state to PLAY.");

						NetworkBuffer *uuid = buffer_new();
						buffer_receive(uuid, socket, sizeof(uint64_t) * 2);

						NetworkBuffer *username = buffer_new();
						buffer_receive_string(username, socket);

						uint32_t no_of_properties = varint_receive(socket);

						NetworkBuffer *properties_array = buffer_new();
						for (int i = 0; i < no_of_properties; ++i) {
							buffer_receive_string(properties_array, socket);
							buffer_receive_string(properties_array, socket);
							bool is_signed = buffer_receive_type(uint8_t);
							buffer_write(properties_array, &is_signed, sizeof(is_signed));

							if (is_signed) {
								buffer_receive_string(properties_array, socket);
							}
						}

						LoginSuccessPacket *packet = login_success_packet_new(
                                uuid, username, varint_encode(no_of_properties), properties_array
						);
//                        packet_decode(&packet->_header);
						packet_event(LOGIN_SUCCESS_PKT, &packet->_header);
						break;
					}
					case SET_COMPRESSION_ID: {
						//TODO: implement packet
						cmc_log(INFO, "Enabled compression.");
						set_compression(true);
						break;
					}
					default:
						cmc_log(DEBUG, "Consumed packet with id %d.", generic_packet->packet_id);
				}
				break;


			case PLAY:
				switch (generic_packet->packet_id) {
					case DISCONNECT_PLAY: {
						DisconnectPlayPacket *packet = disconnect_play_packet_new(NULL);
						NetworkBuffer *response = buffer_new();
						buffer_receive_string(response, socket);
						packet->reason = response;
						packet_event(DISCONNECT_PLAY_PKT, &(packet->_header));
						return;
					}
					case LOGIN_PLAY_ID: {
						cmc_log(INFO, "Received Login Play Packet.");
						LoginPlayPacket *packet = login_play_packet_new(
								0, NULL, 0, 0, NULL, NULL, NULL,
								NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL,
								NULL, NULL, NULL, 0);
						packet_decode(&packet->_header, generic_packet->data);
						packet_event(LOGIN_PLAY_PKT, &packet->_header);
						break;
					}
					case CHANGE_DIFFICULTY_ID: {
						ChangeDifficultyPacket *packet = change_difficulty_packet_new(0, NULL);
						packet_decode(&packet->_header, generic_packet->data);
						cmc_log(INFO, "Set initial difficulty to: %d.", packet->difficulty);
						packet_event(CHANGE_DIFFICULTY_PKT, &packet->_header);
						break;
					}
					case PLAYER_ABILITIES_CB_ID: {
						PlayerAbilitiesCBPacket *packet = player_abilities_cb_packet_new(0, 0, 0);
						packet_decode(&packet->_header, generic_packet->data);
						packet_event(PLAYER_ABILITIES_CB_PKT, &packet->_header);
						break;
					}
					case SYNCHRONIZE_PLAYER_POSITION_ID: {
						SynchronizePlayerPositionPacket *packet = synchronize_player_position_packet_new(
								0, 0, 0, 0, 0, 0, NULL, NULL
						);
						packet_decode(&packet->_header, generic_packet->data);
						packet_event(SYNCHRONIZE_PLAYER_POS_PKT, &packet->_header);
						break;
					}
					case UPDATE_RECIPES_ID: {
						cmc_log(DEBUG, "Recipes packet, size = %d", generic_packet->uncompressed_length);
//                        const char* CRAFTING_SHAPELESS = "crafting_shapeless";
//                        const char* CRAFTING_SHAPED = "crafting_shaped";
//                        const char* OVEN[] = {
//                                "smelting", "blasting", "smoking", "campfire_cooking"
//                        };
//                        const char* STONECUTTING = "stonecutting";
//                        const char* SMITHING = "smithing";
//
//                        uint32_t no_of_recipes = varint_receive(socket);
//                        for (int i = 0; i < no_of_recipes; ++i) {
//                            NetworkBuffer *type = buffer_new();
//                            buffer_receive_string(type, socket);
//                            NetworkBuffer *recipe_id = buffer_new();
//                            buffer_receive_string(type, socket);
//                            if (strcmp(type->bytes, CRAFTING_SHAPELESS) == 0) {
//                                NetworkBuffer *string = buffer_new();
//                                buffer_receive_string(string, socket);
//                                uint32_t ingredients = varint_receive(socket);
//                                for (int j = 0; j < ingredients; ++j) {
//
//                                }
//                                buffer_free(string);
//                            } else if (strcmp(type->bytes, CRAFTING_SHAPED) == 0) {
//
//                            }
//
//                            buffer_free(type);
//                            buffer_free(recipe_id);
//                        }
						break;
					}
					default:
						cmc_log(DEBUG, "Consumed packet with id %d.", generic_packet->packet_id);
				}
				break;


			default:
				cmc_log(ERR, "Invalid connection state (state: %d)!", connectionState);
				return;
		}
	}
    serverstate_free(serverState);
}

