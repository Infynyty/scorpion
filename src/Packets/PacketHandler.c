#include "PacketHandler.h"
#include "VarInt/MCVarInt.h"
#include <stdlib.h>
#include <unistd.h>

#include "NetworkBuffer.h"
#include "Logging/Logger.h"
#include "Packets.h"
#include "NBTParser.h"


// Connection status: STATUS
#define STATUS_RESPONSE                 0x00
#define PING_RESPONSE                   0x01

// Connection status: LOGIN

#define DISCONNECT_LOGIN                0x00
#define LOGIN_SUCCESS_ID                0x02
#define SET_COMPRESSION                 0x03

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


//TODO: abstract method for packet receive using packet fields?
void handle_packets(SocketWrapper *socket, ClientState *clientState) {
	Packets packet_type;
	ConnectionState connectionState = LOGIN;

	while (true) {
		int packet_length_total = varint_receive(socket) - 1; //TODO: Assumes packet ids are always one byte

		if (packet_length_total < 0) {
			continue;
		}
		int packet_id = varint_receive(socket);

		if (packet_id > 1000) {
			cmc_log(ERR, "Illegal packet id %d, packet size: %d", packet_id, packet_length_total);
			continue;
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
					default:
						consume_packet(socket, packet_length_total);
						cmc_log(DEBUG, "Consumed packet with id %d.", packet_id);
				}
				break;


			case LOGIN:
				switch (packet_id) {
					case LOGIN_SUCCESS_ID: {
						cmc_log(DEBUG, "Received Login Success Packet.");
						connectionState = PLAY;
						cmc_log(DEBUG, "Switched connection state to PLAY.");

//						NetworkBuffer *uuid = buffer_new();
//						buffer_receive(uuid, socket, sizeof(uint64_t) * 2);
//
//						NetworkBuffer *username = buffer_new();
//						buffer_receive_string(username, socket);
//
//						uint32_t no_of_properties = varint_receive(socket);
//
//						NetworkBuffer *properties_array = buffer_new();
//						for (int i = 0; i < no_of_properties; ++i) {
//							buffer_receive_string(properties_array, socket);
//							buffer_receive_string(properties_array, socket);
//							bool is_signed = buffer_receive_uint8_t(socket);
//							buffer_write(properties_array, &is_signed, sizeof(is_signed));
//
//							if (is_signed) {
//								buffer_receive_string(properties_array, socket);
//							}
//						}

						LoginSuccessPacket *packet = login_success_packet_new(
								NULL, NULL, NULL, NULL
						);
						packet_receive(&packet->_header);
						packet_event(LOGIN_SUCCESS_PKT, &packet->_header);
						break;
					}
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
					case LOGIN_PLAY_ID: {
						cmc_log(INFO, "Received Login Play Packet.");

						uint32_t entity_id = buffer_receive_uint32_t(socket);
						bool is_hardcore = buffer_receive_uint8_t(socket);
						Gamemode gamemode = buffer_receive_uint8_t(socket);
						Gamemode gamemode_prev = buffer_receive_int8_t(socket);
						MCVarInt *dim_count = writeVarInt(varint_receive(socket));

						NetworkBuffer *dim_identifiers = buffer_new();
						for (int i = 0; i < varint_decode(dim_count->bytes); ++i) {
							buffer_receive_string(dim_identifiers, socket);
						}

						NetworkBuffer *registry = buffer_new();
						consume_nbt_data(socket);

						NetworkBuffer *dim_type = buffer_new();
						buffer_receive_string(dim_type, socket);

						NetworkBuffer *dim_name = buffer_new();
						buffer_receive_string(dim_name, socket);

						uint64_t hashed_seed = buffer_receive_uint64_t(socket);
						uint32_t max_players = varint_receive(socket);
						uint32_t view_distance = varint_receive(socket);
						uint32_t simulation_distance = varint_receive(socket);

						bool reduced_debug_info = buffer_receive_uint8_t(socket);
						bool enable_respawn_screen = buffer_receive_uint8_t(socket);
						bool is_debug = buffer_receive_uint8_t(socket);
						bool is_flat = buffer_receive_uint8_t(socket);
						bool has_death_location = buffer_receive_uint8_t(socket);

						NetworkBuffer *death_dim_name = NULL;
						uint64_t death_position;

						if (has_death_location) {
							death_dim_name = buffer_new();
							buffer_receive_string(death_dim_name, socket);
							death_position = buffer_receive_uint64_t(socket);
						}

						LoginPlayPacket *packet = login_play_packet_new(
								entity_id,
								(uint8_t) is_hardcore,
								(int8_t) gamemode,
								gamemode_prev,
								dim_count,
								dim_identifiers,
								registry,
								dim_name,
								dim_type,
								hashed_seed,
								writeVarInt(max_players),
								writeVarInt(view_distance),
								writeVarInt(simulation_distance),
								reduced_debug_info,
								enable_respawn_screen,
								is_debug,
								is_flat,
								has_death_location,
								death_dim_name,
								death_position
						);
						packet_event(LOGIN_PLAY_PKT, &packet->_header);
						break;
					}
					case CHANGE_DIFFICULTY_ID: {
						Difficulty difficulty = buffer_receive_uint8_t(socket);
						bool difficulty_locked = buffer_receive_uint8_t(socket);

						ChangeDifficultyPacket *packet = change_difficulty_packet_new(difficulty, difficulty_locked);

						packet_event(CHANGE_DIFFICULTY_PKT, &packet->_header);
						break;
					}
					case PLAYER_ABILITIES_CB_ID: {
						uint8_t flags = buffer_receive_uint8_t(socket);
						float flying_speed = buffer_receive_float(socket);
						float fov_modifier = buffer_receive_float(socket);

						PlayerAbilitiesCBPacket *packet = player_abilities_cb_packet_new(flags, flying_speed,
						                                                                 fov_modifier);

						packet_event(PLAYER_ABILITIES_CB_PKT, &packet->_header);
						break;
					}
					case SYNCHRONIZE_PLAYER_POSITION_ID: {
						double x = buffer_receive_double(socket);
						double y = buffer_receive_double(socket);
						double z = buffer_receive_double(socket);
						float yaw = buffer_receive_float(socket);
						float pitch = buffer_receive_float(socket);
						uint8_t flags = buffer_receive_uint8_t(socket);
						MCVarInt *teleport_id = writeVarInt(varint_receive(socket));
						bool dismount_vehicle = buffer_receive_uint8_t(socket);

						SynchronizePlayerPositionPacket *packet = synchronize_player_position_packet_new(
								x,
								y,
								z,
								yaw,
								pitch,
								flags,
								teleport_id,
								dismount_vehicle
						);
						packet_event(SYNCHRONIZE_PLAYER_POS_PKT, &packet->_header);
						break;
					}
					case UPDATE_RECIPES_ID: {
						cmc_log(DEBUG, "Recipes packet, length = %d", packet_length_total);
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

						consume_packet(socket, packet_length_total);
						break;
					}
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
	NetworkBuffer *buffer = buffer_new();
	buffer_receive(buffer, socket, length_in_bytes);
	buffer_free(buffer);
}

