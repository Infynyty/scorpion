#include <stdlib.h>
#include "Packets.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"
#include "Logger.h"
#include "NBTParser.h"


uint8_t get_types_size(PacketField type) {
	switch (type) {
		case PKT_BOOL:
			return sizeof(uint8_t);
		case PKT_BYTE:
			return sizeof(int8_t);
		case PKT_UINT8:
			return sizeof(uint8_t);
		case PKT_UINT16:
			return sizeof(uint16_t);
		case PKT_UINT32:
			return sizeof(uint32_t);
		case PKT_UINT64:
			return sizeof(uint64_t);
		case PKT_FLOAT:
			return sizeof(float);
		case PKT_DOUBLE:
			return sizeof(double);
		default:
			return sizeof(void *);
	}
}

void packet_send(PacketHeader *packet, SocketWrapper *socket) {
	NetworkBuffer *buffer = buffer_new();
	void *current_byte = packet + 1;
	buffer_write_little_endian(buffer, packet->packet_id->bytes, packet->packet_id->length);
	for (int i = 0; i < packet->members; ++i) {
		PacketField m_type = packet->member_types[i];

		bool *is_optional = packet->optionals[i];
		if (is_optional == NULL || *is_optional) {
			switch (m_type) {
				case PKT_BOOL:
				case PKT_UINT8:
				case PKT_UINT16:
				case PKT_UINT32:
				case PKT_UINT64:
				case PKT_FLOAT:
				case PKT_DOUBLE:
					buffer_write(buffer, current_byte, get_types_size(m_type));
					break;
				case PKT_VARINT: {
					MCVarInt *varInt = (*(MCVarInt **) (current_byte));
					buffer_write_little_endian(buffer, varInt->bytes, varInt->length);
					break;
				}
				case PKT_VARLONG:
					//TODO: Fill in
					break;
				case PKT_BYTEARRAY:
				case PKT_UUID:
				case PKT_STRING: {
					NetworkBuffer *string = *((NetworkBuffer **) current_byte);
					MCVarInt *length = varint_new(string->byte_size);
					buffer_write_little_endian(buffer, length->bytes, length->length);
					buffer_write_little_endian(buffer, string->bytes, string->byte_size);


					free(length);
					break;
				}
				default:
					cmc_log(ERR, "Used unsupported type at packet_send(): %d", m_type);
					exit(EXIT_FAILURE);
			}
		}
		current_byte = (void *) current_byte;
		current_byte += get_types_size(m_type);
	}
	buffer_send_packet(buffer, socket);
	buffer_free(buffer);
}

void packet_free(PacketHeader *packet) {
	void *ptr = packet + 1;
	for (int i = 0; i < packet->members; ++i) {
		PacketField m_type = packet->member_types[i];
		bool *optional_present = packet->optionals[i];
		if (optional_present != NULL && !*optional_present) continue;
		switch (m_type) {
			case PKT_BOOL:
			case PKT_BYTE:
			case PKT_UINT8:
			case PKT_UINT16:
			case PKT_UINT32:
			case PKT_UINT64:
			case PKT_FLOAT:
			case PKT_DOUBLE:
				ptr += get_types_size(m_type);
				break;
			case PKT_VARINT: {
				MCVarInt **varInt = (MCVarInt **) ptr;
				free(*varInt);
				varInt++;
				ptr = varInt;
				break;
			}
			case PKT_ARRAY:
			case PKT_BYTEARRAY:
			case PKT_STRING_ARRAY:
			case PKT_UUID:
			case PKT_NBTTAG:
			case PKT_STRING: {
				NetworkBuffer **string = (NetworkBuffer **) ptr;
				buffer_free(*string);
				string++;
				ptr = string;
				break;
			}
            case PKT_PREV_MESS_ARRAY: {
                MCVarInt **length_varint = (MCVarInt **) ptr;
                uint8_t length = varint_decode(get_bytes(*length_varint));
                free(*length_varint);
                length_varint++;
                ptr = length_varint;

                for (int j = 0; j < length; ++j) {
                    NetworkBuffer **sender_uuid = (NetworkBuffer **) ptr;
                    buffer_free(*sender_uuid);
                    sender_uuid++;
                    ptr = sender_uuid;

                    NetworkBuffer **signature = (NetworkBuffer **) ptr;
                    buffer_free(*signature);
                    signature++;
                    ptr = signature;
                }
                break;
            }
//            case PKT_ARRAY: {
//                MCVarInt **varInt = (MCVarInt **) ptr;
//                uint32_t length = varint_decode(get_bytes(*varInt));
//                free(*varInt);
//                varInt++;
//                ptr = varInt;
//                for (int j = 0; j < length; ++j) {
//                    NetworkBuffer **string = (NetworkBuffer **) ptr;
//                    buffer_free(*string);
//                    string++;
//                    ptr = string;
//                }
//                break;
//            }
			case PKT_VARLONG:
			case PKT_IDENTIFIER:
			case PKT_ENTITYMETA:
			case PKT_SLOT:
			case PKT_OPTIONAL:
			case PKT_ENUM:
			case PKT_CHAT:
			default:
				cmc_log(ERR, "Used unsupported type at packet_free(): %d", m_type);
				exit(EXIT_FAILURE);
		}
	}

	free(packet->optionals);
	free(packet->member_types);
	free(packet->packet_id);
	free(packet);
}

void packet_receive(PacketHeader *header) {
	void *ptr = &header + 1;
	for (int i = 0; i < header->members; ++i) {
		PacketField field = header->member_types[i];

		bool *is_optional = header->optionals[i];
		if (is_optional != NULL && !*is_optional) {
            ptr += sizeof(void *);
            continue;
        }

		void *variable_pointer;
		size_t variable_size;
		switch (field) {
			case PKT_BOOL:
			case PKT_BYTE:
			case PKT_UINT8: {
				uint8_t uint8 = buffer_receive_uint8_t(get_socket());
				variable_pointer = &uint8;
				variable_size = sizeof(uint8_t);
				break;
			}
			case PKT_UINT16: {
				uint16_t uint16 = buffer_receive_int16_t(get_socket());
				variable_pointer = &uint16;
				variable_size = sizeof(uint16_t);
				break;
			}
			case PKT_UINT32: {
				uint32_t uint32 = buffer_receive_uint32_t(get_socket());
				variable_pointer = &uint32;
				variable_size = sizeof(uint32_t);
				break;
			}
			case PKT_UINT64: {
				uint64_t uint64 = buffer_receive_uint64_t(get_socket());
				variable_pointer = &uint64;
				variable_size = sizeof(uint64_t);
				break;
			}
			case PKT_FLOAT: {
				float number = buffer_receive_float(get_socket());
				variable_pointer = &number;
				variable_size = sizeof(float);
				break;
			}
			case PKT_DOUBLE: {
				double number = buffer_receive_double(get_socket());
				variable_pointer = &number;
				variable_size = sizeof(double);
				break;
			}
			case PKT_VARINT: {
				MCVarInt *var_int = varint_new(varint_receive(get_socket()));
				variable_pointer = &var_int;
				variable_size = sizeof(MCVarInt *);
				break;
			}
			case PKT_UUID: {
				NetworkBuffer *uuid = buffer_new();
				buffer_receive(uuid, get_socket(), 2 * sizeof(uint64_t));
				variable_pointer = &uuid;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_BYTEARRAY: {
				NetworkBuffer *bytes = buffer_new();
				uint32_t length = varint_receive(get_socket());
				buffer_receive(bytes, get_socket(), length);
				variable_pointer = &bytes;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_STRING_ARRAY: {
				NetworkBuffer *strings = buffer_new();
				uint32_t length = varint_receive(get_socket());
				for (int j = 0; j < length; ++j) {
					buffer_receive_string(strings, get_socket());
				}
				variable_pointer = &strings;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_STRING: {
				NetworkBuffer *string = buffer_new();
				buffer_receive_string(string, get_socket());
				variable_pointer = &string;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_NBTTAG: {
				NetworkBuffer *nbt = buffer_new();
				consume_nbt_data(get_socket());
				variable_pointer = &nbt;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
            case PKT_PREV_MESS_ARRAY: {
                NetworkBuffer *test = buffer_new();
//                uint8_t testNum = buffer_receive_uint8_t(get_socket());
//                buffer_receive(test, get_socket(), 30);

                uint8_t length = varint_receive(get_socket());
                MCVarInt *length_varint = varint_new(length);
                NetworkBuffer **uuids = malloc(length * sizeof(NetworkBuffer *));
                NetworkBuffer **signatures = malloc(length * sizeof(NetworkBuffer *));

                for (int j = 0; j < length; ++j) {
                    NetworkBuffer *uuid = buffer_new();
                    buffer_receive(uuid, get_socket(), 2 * sizeof(uint64_t));
                    uuids[j] = uuid;

                    NetworkBuffer *bytes = buffer_new();
                    uint32_t byte_length = varint_receive(get_socket());
                    buffer_receive(bytes, get_socket(), byte_length);
                    signatures[j] = bytes;
                }
                memmove(ptr, &length_varint, sizeof(MCVarInt *));
                ptr += sizeof(MCVarInt *);
                memmove(ptr, &uuids, sizeof(NetworkBuffer **));
                ptr += sizeof(NetworkBuffer **);
                variable_pointer = signatures;
                variable_size = sizeof(NetworkBuffer **);
                break;
            }
			case PKT_CHAT:
			case PKT_IDENTIFIER:
			case PKT_VARLONG:
			case PKT_ENTITYMETA:
			case PKT_SLOT:
			case PKT_OPTIONAL:
			case PKT_ARRAY:
			case PKT_ENUM:
			default:
				cmc_log(ERR, "Tried receiving unhandled packet field %d", field);
				exit(EXIT_FAILURE);
		}
		memmove(ptr, variable_pointer, variable_size);
		ptr += variable_size;
	}
}

PacketHeader *create_header(uint8_t no_of_fields, uint8_t packet_id) {
    PacketField *fields = malloc(no_of_fields * sizeof(PacketField));
    bool **optionals = calloc(no_of_fields, sizeof(bool *));
    PacketHeader *header = malloc(sizeof(PacketHeader));
    header->members = no_of_fields;
    header->member_types = fields;
    header->optionals = optionals;
    header->direction = SERVERBOUND;
    header->state = STATUS;
    return header;
}

HandshakePacket *handshake_pkt_new(NetworkBuffer *address, unsigned short port, HandshakeNextState state) {
	HandshakePacket *packet = malloc(sizeof(HandshakePacket));
	uint8_t types_length = 4;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_VARINT,
			PKT_STRING,
			PKT_UINT16,
			PKT_VARINT
	};
	bool **optionals = calloc(types_length, sizeof(bool *));
	memmove(types, typeArray, types_length * sizeof(PacketField));
	MCVarInt *packet_id = varint_new(0x00);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = SERVERBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->protocol_version = varint_new(760);
	packet->address = address;
	packet->port = port;
	packet->next_state = varint_new(state);
	return packet;
}

StatusRequestPacket *status_request_packet_new() {
	StatusRequestPacket *packet = malloc(sizeof(StatusRequestPacket));
	uint8_t types_length = 0;
	PacketField *types = NULL;
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = SERVERBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	return packet;
}

StatusResponsePacket *status_response_packet_new(NetworkBuffer *response) {
	StatusResponsePacket *packet = malloc(sizeof(StatusResponsePacket));
	uint8_t types_length = 1;
	PacketField *types = malloc(1 * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_STRING
	};
	bool **optionals = calloc(types_length, sizeof(bool *));
	memmove(types, typeArray, types_length * sizeof(PacketField));
	MCVarInt *packet_id = varint_new(0x00);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->response = response;
	return packet;
}

LoginStartPacket *login_start_packet_new(NetworkBuffer *player_name, bool has_sig_data, bool has_player_uuid) {
	LoginStartPacket *packet = malloc(sizeof(LoginStartPacket));
	uint8_t types_length = 7;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_STRING,
			PKT_BOOL,
			PKT_UINT64,
			PKT_BYTEARRAY,
			PKT_BYTEARRAY,
			PKT_BOOL,
			PKT_UUID
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	if (!has_sig_data) {
		optionals[2] = &packet->has_sig_data;
		optionals[3] = &packet->has_sig_data;
		optionals[4] = &packet->has_sig_data;
	}
	if (!has_player_uuid) {
		optionals[6] = &packet->has_player_uuid;
	}
	MCVarInt *packet_id = varint_new(0x00);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->player_name = player_name;
	packet->has_sig_data = has_sig_data;
	packet->has_player_uuid = has_player_uuid;
	return packet;
}

EncryptionResponsePacket *encryption_response_packet_new(
		NetworkBuffer *shared_secret,
		bool has_verify_token,
		NetworkBuffer *verify_token,
		uint64_t salt,
		NetworkBuffer *message_signature
) {
	EncryptionResponsePacket *packet = malloc(sizeof(EncryptionResponsePacket));
	uint8_t types_length = 3;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_BYTEARRAY,
			PKT_BOOL,
			PKT_BYTEARRAY,
			PKT_UINT64,
			PKT_BYTEARRAY
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));

	optionals[2] = &packet->has_verify_token;
	optionals[3] = &packet->_has_no_verify_token;
	optionals[4] = &packet->_has_no_verify_token;

	MCVarInt *packet_id = varint_new(0x01);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->shared_secret = shared_secret;
	packet->has_verify_token = has_verify_token;
	packet->verify_token = verify_token;
	packet->_has_no_verify_token = !has_verify_token;
	packet->salt = salt;
	packet->message_signature = message_signature;
	return packet;
}


EncryptionRequestPacket *encryption_request_packet_new(
		NetworkBuffer *server_id,
		NetworkBuffer *public_key,
		NetworkBuffer *verify_token
) {
	EncryptionRequestPacket *packet = malloc(sizeof(EncryptionRequestPacket));
	uint8_t types_length = 3;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_STRING,
			PKT_BYTEARRAY,
			PKT_BYTEARRAY
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x01);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->server_id = server_id;
	packet->public_key = public_key;
	packet->verify_token = verify_token;
	return packet;
}

// TODO: implement arrays
LoginSuccessPacket *login_success_packet_new(
		NetworkBuffer *uuid,
		NetworkBuffer *username,
		MCVarInt *number_of_properties,
		NetworkBuffer *properties
) {
	LoginSuccessPacket *packet = malloc(sizeof(LoginSuccessPacket));
	uint8_t types_length = 4;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_UUID,
			PKT_STRING,
			PKT_VARINT,
			PKT_BYTEARRAY
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x02);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->uuid = uuid;
	packet->username = username;
	packet->number_of_properties = number_of_properties;
	packet->properties = properties;
	return packet;
}

ClientInformationPacket *client_info_packet_new(
		NetworkBuffer *locale,
		uint8_t view_distance,
		MCVarInt *chat_mode,
		bool chat_colors,
		uint8_t displayed_skin_parts,
		MCVarInt *main_hand,
		bool enable_text_filtering,
		bool allow_server_listings
) {
	ClientInformationPacket *packet = malloc(sizeof(ClientInformationPacket));
	uint8_t types_length = 8;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_STRING,
			PKT_UINT8,
			PKT_VARINT,
			PKT_BOOL,
			PKT_UINT8,
			PKT_VARINT,
			PKT_BOOL,
			PKT_BOOL
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x08);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->locale = locale;
	packet->view_distance = view_distance;
	packet->chat_mode = chat_mode;
	packet->chat_colors = chat_colors;
	packet->displayed_skin_parts = displayed_skin_parts;
	packet->main_hand = main_hand;
	packet->enable_text_filtering = enable_text_filtering;
	packet->allow_server_listings = allow_server_listings;
	return packet;
}

SetPlayerPosAndRotPacket *set_player_pos_and_rot_packet_new(
		double x,
		double y,
		double z,
		float yaw,
		float pitch,
		bool on_ground
) {
	SetPlayerPosAndRotPacket *packet = malloc(sizeof(SetPlayerPosAndRotPacket));
	uint8_t types_length = 6;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_DOUBLE,
			PKT_DOUBLE,
			PKT_DOUBLE,
			PKT_FLOAT,
			PKT_FLOAT,
			PKT_BOOL
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x15);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->x = x;
	packet->y = y;
	packet->z = z;
	packet->yaw = yaw;
	packet->pitch = pitch;
	packet->on_ground = on_ground;
	return packet;
}

ClientCommandPacket *client_command_packet_new(MCVarInt *action) {
	ClientCommandPacket *packet = malloc(sizeof(ClientCommandPacket));
	uint8_t types_length = 1;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_VARINT
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x07);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->action = action;
	return packet;
}

ConfirmTeleportationPacket *confirm_teleportation_packet_new(MCVarInt *teleport_id) {
	ConfirmTeleportationPacket *packet = malloc(sizeof(ConfirmTeleportationPacket));
	uint8_t types_length = 1;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_VARINT
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x00);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = SERVERBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->teleport_id = teleport_id;
	return packet;
}

LoginPlayPacket *login_play_packet_new(
		uint32_t entity_id,
		bool is_hardcore,
		uint8_t gamemode,
		uint8_t previous_gamemode,
		NetworkBuffer *dimensions,
		NetworkBuffer *registry_codec,
		NetworkBuffer *spawn_dimension_name,
		NetworkBuffer *spawn_dimension_type,
		uint64_t hashed_seed,
		MCVarInt *_max_players,
		MCVarInt *view_distance,
		MCVarInt *simulation_distance,
		bool reduced_debug_info,
		bool enable_respawn_screen,
		bool is_debug,
		bool is_flat,
		bool has_death_location,
		NetworkBuffer *death_dimension_name,
		uint64_t death_location
) {
	LoginPlayPacket *packet = malloc(sizeof(LoginPlayPacket));
	uint8_t types_length = 19;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_UINT32,
			PKT_BOOL,
			PKT_UINT8,
			PKT_UINT8,
			PKT_STRING_ARRAY,
			PKT_NBTTAG,
			PKT_STRING,
			PKT_STRING,
			PKT_UINT64,
			PKT_VARINT,
			PKT_VARINT,
			PKT_VARINT,
			PKT_BOOL,
			PKT_BOOL,
			PKT_BOOL,
			PKT_BOOL,
			PKT_BOOL,
			PKT_STRING,
			PKT_UINT64
	};
	bool **optionals = calloc(types_length, sizeof(bool *));
	if (has_death_location) {
		optionals[17] = &packet->has_death_location;
		optionals[18] = &packet->has_death_location;
	}
	memmove(types, typeArray, types_length * sizeof(PacketField));
	MCVarInt *packet_id = varint_new(0x25);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->entity_id = entity_id;
	packet->is_hardcore = is_hardcore;
	packet->gamemode = (uint8_t) gamemode;
	packet->previous_gamemode = (int8_t) previous_gamemode;
	packet->dimensions = dimensions;
	packet->registry_codec = registry_codec;
	packet->spawn_dimension_name = spawn_dimension_name;
	packet->spawn_dimension_type = spawn_dimension_type;
	packet->hashed_seed = hashed_seed;
	packet->_max_players = _max_players;
	packet->view_distance = view_distance;
	packet->simulation_distance = simulation_distance;
	packet->reduced_debug_info = reduced_debug_info;
	packet->enable_respawn_screen = enable_respawn_screen;
	packet->is_debug = is_debug;
	packet->is_flat = is_flat;
	packet->has_death_location = has_death_location;
	packet->death_dimension_name = death_dimension_name;
	packet->death_location = death_location;
	return packet;
}


DisconnectPlayPacket *disconnect_play_packet_new(NetworkBuffer *reason) {
	DisconnectPlayPacket *packet = malloc(sizeof(DisconnectPlayPacket));
	uint8_t types_length = 1;
	PacketField *types = malloc(1 * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_STRING
	};
	bool **optionals = calloc(types_length, sizeof(bool *));
	memmove(types, typeArray, types_length * sizeof(PacketField));
	MCVarInt *packet_id = varint_new(0x19);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->reason = reason;
	return packet;
}

SynchronizePlayerPositionPacket *synchronize_player_position_packet_new(
		double x,
		double y,
		double z,
		float yaw,
		float pitch,
		uint8_t flags,
		MCVarInt *teleport_id,
		bool dismount_vehicle
) {
	SynchronizePlayerPositionPacket *packet = malloc(sizeof(SynchronizePlayerPositionPacket));
	uint8_t types_length = 8;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_DOUBLE,
			PKT_DOUBLE,
			PKT_DOUBLE,
			PKT_FLOAT,
			PKT_FLOAT,
			PKT_UINT8,
			PKT_VARINT,
			PKT_BOOL
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x39);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->x = x;
	packet->y = y;
	packet->z = z;
	packet->yaw = yaw;
	packet->pitch = pitch;
	packet->flags = flags;
	packet->teleport_id = teleport_id;
	packet->dismount_vehicle = dismount_vehicle;
	return packet;
}

UpdateRecipesPacket *update_recipes_packet_new(MCVarInt *no_of_recipes, NetworkBuffer *recipes) {
	UpdateRecipesPacket *packet = malloc(sizeof(UpdateRecipesPacket));
	uint8_t types_length = 2;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_VARINT,
			PKT_STRING
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x39);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->no_of_recipes = no_of_recipes;
	packet->recipe = recipes;
	return packet;
}

ChangeDifficultyPacket *change_difficulty_packet_new(uint8_t difficulty, bool difficulty_locked) {
	ChangeDifficultyPacket *packet = malloc(sizeof(ChangeDifficultyPacket));
	uint8_t types_length = 2;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_UINT8,
			PKT_BOOL
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x0b);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->difficulty = difficulty;
	packet->difficulty_locked = difficulty_locked;
	return packet;
}

PlayerAbilitiesCBPacket *player_abilities_cb_packet_new(uint8_t flags, float flying_speed, float fov_modifier) {
	PlayerAbilitiesCBPacket *packet = malloc(sizeof(PlayerAbilitiesCBPacket));
	uint8_t types_length = 3;
	PacketField *types = malloc(types_length * sizeof(PacketField));
	PacketField typeArray[] = {
			PKT_UINT8,
			PKT_FLOAT,
			PKT_FLOAT
	};
	memmove(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_new(0x31);
	PacketHeader header = {
			.member_types = types,
			.members = types_length,
			.optionals = optionals,
			.direction = CLIENTBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = header;
	packet->flags = flags;
	packet->flying_speed = flying_speed;
	packet->fov_modifier = fov_modifier;
	return packet;
}

PlayerChatMessagePacket *player_chat_message_packet_new(
//        bool has_message_signature,
//        NetworkBuffer *message_signature,
//        NetworkBuffer *uuid,
//        NetworkBuffer *header_signature,
//        NetworkBuffer *plain_message,
//        bool has_formatting,
//        NetworkBuffer *formatting,
//        uint64_t timestamp,
//        uint64_t salt,
//
//        MCVarInt *previous_messages_length,
//        NetworkBuffer **previous_sender_uuids,
//        NetworkBuffer **previous_signatures,
//
//        bool has_unsigned_content,
//        NetworkBuffer *unsigned_content,
//        MCVarInt *filter_type,
//        NetworkBuffer *filter_mask,
//        MCVarInt *chat_type,
//        NetworkBuffer *network_name,
//        bool has_network_target_name,
//        NetworkBuffer *network_target_name
) {
    PlayerChatMessagePacket *packet_ptr = malloc(sizeof(PlayerChatMessagePacket));
    PacketHeader *header = create_header(18, 0x33);
    header->member_types = (PacketField []) {
            PKT_BOOL,
            PKT_BYTEARRAY,
            PKT_UUID,
            PKT_BYTEARRAY,
            PKT_STRING,
            PKT_BOOL,
            PKT_STRING,
            PKT_UINT64,
            PKT_UINT64,
            PKT_PREV_MESS_ARRAY,
            PKT_BOOL,
            PKT_STRING,
            PKT_VARINT,
            PKT_BITARRAY,
            PKT_VARINT,
            PKT_STRING,
            PKT_BOOL,
            PKT_STRING
    };
    header->optionals[1] = &packet_ptr->has_message_signature;
    header->optionals[6] = &packet_ptr->has_formatting;
    header->optionals[11] = &packet_ptr->has_unsigned_content;
    header->optionals[17] = &packet_ptr->has_network_target_name;

    packet_ptr->_header = header;
}
