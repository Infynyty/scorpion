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
#include "Authentication.h"
#include "PlayState.h"
#include <pthread.h>
#include "AStar.h"
#include "Movement.h"


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
#define PLAYER_ABILITIES_CB_ID          0x30
#define SYNCHRONIZE_PLAYER_POSITION_ID  0x38
#define DISCONNECT_PLAY                 0x17
#define LOGIN_PLAY_ID                   0x24
#define SET_HELD_ITEM_ID                0x4a
#define UPDATE_RECIPES_ID               0x69
#define KEEP_ALIVE_CLIENTBOUND_ID       0x1f
#define PLAYER_CHAT_MESSAGE_ID          0x31
#define CHUNK_DATA_ID                   0x20
#define UNLOAD_CHUNK_ID                 0x1b

void consume_packet(SocketWrapper *socket, int length_in_bytes);

typedef void (*HandlePacket)(void *packet, PlayState *state);

typedef struct HandlerNode {
	int id;

	void (*handle)(void *packet, PlayState *state);

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

int register_handler(void (*handle)(void *packet, PlayState *state), Packets packet_type) {
	return add_node(handle, packet_type);
}

void packet_event(Packets packet_type, PacketHeader **packet, PlayState *state) {
	HandlerNode *current_node = packet_handlers[packet_type];
	while (current_node != NULL) {
		(*current_node->handle)(packet, state);
		current_node = current_node->next;
	}
    cmc_log(INFO, "Freeing packet after event: id %d", varint_decode((*packet)->packet_id->bytes));
	packet_free(packet);
}

GenericPacket *get_packet(ConnectionState state, PacketHandleWrapper *wrapper) {
    if (state == PLAY) {
        pthread_mutex_lock(wrapper->list->mutex);
        while (wrapper->list->first == NULL) {
            pthread_cond_wait(wrapper->list->condition, wrapper->list->mutex);
        }
        GenericPacketList *list = wrapper->list;
        GenericPacket *generic_packet = list->first;
        if (list->first == list->last) {
            list->first = NULL;
            list->last = NULL;
        } else {
            list->first = list->first->next;
        }
        pthread_mutex_unlock(list->mutex);
        return generic_packet;
    } else {
        return packet_receive_single();
    }
}

void *handle_packets(void *wrapper_arg) {
	ConnectionState connectionState = LOGIN;
    PacketHandleWrapper *wrapper = wrapper_arg;
    GenericPacketList *list = wrapper->list;
    PlayState *state = wrapper->state;

	while (true) {

        GenericPacket *generic_packet = get_packet(connectionState, wrapper);
        if (generic_packet == NULL) continue;

		if (generic_packet->uncompressed_length < 0) {
            generic_packet_free(generic_packet);
			continue;
		}

		if (generic_packet->packet_id > 255) {
			cmc_log(ERR, "Illegal packet id %d, packet size: %d", generic_packet->packet_id,
			        generic_packet->uncompressed_length);
            generic_packet_free(generic_packet);
			continue;
		}
		switch (connectionState) {
			case STATUS:
				switch (generic_packet->packet_id) {
					case STATUS_RESPONSE: {
						StatusResponsePacket packet = {._header = status_response_packet_new()};
						packet_decode(&packet._header, generic_packet->data);
						packet_event(STATUS_RESPONSE_PKT, &packet._header, state);
						break;
					}
					default:
						cmc_log(DEBUG, "Consumed packet with id %d.", generic_packet->packet_id);
				}
				break;


			case LOGIN:
				switch (generic_packet->packet_id) {
					case DISCONNECT_LOGIN: {
						DisconnectLoginPacket packet = {._header = disconnect_login_packet_new()};
						packet_decode(&packet._header, generic_packet->data);
						packet_event(LOGIN_DISCONNECT_PKT, &packet._header, state);
                        return NULL;
					}
					case ENCRYPTION_REQUEST_ID: {
						cmc_log(INFO, "Received encryption request.");
						EncryptionRequestPacket packet = {._header = encryption_request_packet_new()};
						packet_decode(&packet._header, generic_packet->data);

						state->serverState->public_key = packet.public_key;
						state->serverState->verify_token = packet.verify_token;

						NetworkBuffer *secret = buffer_new();
                        EncryptionResponsePacket response = {._header = encryption_response_packet_new()};
                        encryption_response_generate(&response,
                                                     packet.public_key,
                                                     packet.verify_token,
                                                     secret
                        );
                        authenticate_server(&packet, secret, state->clientState);
						packet_send(&response._header);
                        packet_free(&response._header);
						cmc_log(INFO, "Sent encryption response.");
						init_encryption(secret);
						buffer_free(secret);

						packet_event(ENCRYPTION_REQUEST_PKT, &packet._header, state);
						break;
					}
					case LOGIN_SUCCESS_ID: {
                        LoginSuccessPacket packet = {._header = login_success_packet_new()};
                        packet_decode(&packet._header, generic_packet->data);
						cmc_log(DEBUG, "Received Login Success Packet.");
						connectionState = PLAY;
						cmc_log(DEBUG, "Switched connection state to PLAY.");

                        cmc_log(DEBUG, "Enabling multi-threaded packet receiving.");
                        pthread_create(wrapper->receive_thread, NULL, packet_receive, list);

						packet_event(LOGIN_SUCCESS_PKT, &packet._header, state);
						break;
					}
					case SET_COMPRESSION_ID: {
						SetCompressionPacket packet = {._header = set_compression_packet_new()};
                        packet_decode(&packet._header, generic_packet->data);

                        set_compression_threshold(varint_decode(packet.threshold->bytes));
						cmc_log(INFO, "Enabled compression.");
                        packet_event(SET_COMPRESSION_PKT, &packet._header, state);
						break;
					}
					default:
						cmc_log(DEBUG, "Consumed packet with id %d.", generic_packet->packet_id);
				}
				break;


			case PLAY:
				switch (generic_packet->packet_id) {
					case DISCONNECT_PLAY: {
                        cmc_log(INFO, "Received Disconnect play packet.");
                        DisconnectPlayPacket packet = {._header = disconnect_play_packet_new()};
                        packet_decode(&packet._header, generic_packet->data);
                        packet_event(DISCONNECT_PLAY_PKT, &packet._header, state);
						return NULL;
					}
					case LOGIN_PLAY_ID: {
						cmc_log(INFO, "Received Login Play Packet.");
						LoginPlayPacket packet = {._header = login_play_packet_new(&packet.has_death_location)};
						packet_decode(&packet._header, generic_packet->data);
						packet_event(LOGIN_PLAY_PKT, &packet._header, state);
						break;
					}
					case CHANGE_DIFFICULTY_ID: {
						ChangeDifficultyPacket packet = {._header = change_difficulty_packet_new()};
						packet_decode(&packet._header, generic_packet->data);
						cmc_log(INFO, "Set initial difficulty to: %d.", packet.difficulty);
						packet_event(CHANGE_DIFFICULTY_PKT, &packet._header, state);
						break;
					}
					case PLAYER_ABILITIES_CB_ID: {
						PlayerAbilitiesCBPacket packet = {._header = player_abilities_cb_packet_new()};
						packet_decode(&packet._header, generic_packet->data);
						packet_event(PLAYER_ABILITIES_CB_PKT, &packet._header, state);
						break;
					}
					case SYNCHRONIZE_PLAYER_POSITION_ID: {
						SynchronizePlayerPositionPacket packet = {._header = synchronize_player_position_packet_new()};
						packet_decode(&packet._header, generic_packet->data);
						packet_event(SYNCHRONIZE_PLAYER_POS_PKT, &packet._header, state);
						break;
					}
                    case CHUNK_DATA_ID: {
                        ChunkDataPacket packet = {._header = chunk_data_packet_new()};
                        packet_decode(&packet._header, generic_packet->data);
                        packet_event(CHUNK_DATA_PKT, &packet._header, state);
                        break;
                    }
                    case UNLOAD_CHUNK_ID: {
                        UnloadChunkPacket packet = {._header = unload_chunk_packet_new()};
                        packet_decode(&packet._header, generic_packet->data);
                        packet_event(UNLOAD_CHUNK_PKT, &packet._header, state);
                        break;
                    }
                    case KEEP_ALIVE_CLIENTBOUND_ID: {

                        KeepAliveClientboundPacket packet = {._header = keep_alive_clientbound_packet_new()};
                        packet_decode(&packet._header, generic_packet->data);
                        packet_event(KEEP_ALIVE_CLIENTBOUND_PKT, &packet._header, state);
                        break;
                    }
                    case PLAYER_CHAT_MESSAGE_ID: {
                        uint32_t counter = 0;
                        Position ** positions = find_path(state->clientState->position, position_new(0, -61, 4, 0, 0), state->worldState, &counter);
                        for (int i = 0; i < counter; ++i) {
                            move_player(positions[i], state);
                        }
                        PlayerChatMessagePacket packet = {._header = player_chat_message_header(
                                &packet.has_message_signature,
                                &packet.has_unsigned_content,
                                &packet.network_target_name_present
                                )};
                        packet_decode(&packet._header, generic_packet->data);
                        packet_event(PLAYER_CHAT_MESSAGE_PKT, &packet._header, state);
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
				return NULL;
		}
        generic_packet_free(generic_packet);
	}
}

