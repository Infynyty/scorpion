//
// Created by Kasimir on 04.11.2022.
//

#include <stdlib.h>
#include "Packets.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"
#include "Logger.h"


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

        if (!packet->retain_types[i]) {
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
                    MCVarInt *length = writeVarInt(string->byte_size);
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
        if (packet->retain_types[i]) continue;
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
            case PKT_UUID:
			case PKT_STRING: {
				NetworkBuffer **string = (NetworkBuffer **) ptr;
				buffer_free(*string);
				string++;
				ptr = string;
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
			case PKT_NBTTAG:
			case PKT_OPTIONAL:
			case PKT_ENUM:
			case PKT_CHAT:
			default:
				cmc_log(ERR, "Used unsupported type at packet_free(): %d", m_type);
				exit(EXIT_FAILURE);
		}
	}
	free(packet->member_types);
	free(packet->packet_id);
	free(packet);
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
    bool *retain_element = calloc(types_length, sizeof(bool));
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	MCVarInt *packet_id = writeVarInt(0x00);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
            .retain_types = retain_element,
			.direction = SERVERBOUND,
			.state = STATUS,
			.packet_id = packet_id
	};
	packet->_header = wrapper;
	packet->protocol_version = writeVarInt(760);
	packet->address = address;
	packet->port = port;
	packet->next_state = writeVarInt(state);
	return packet;
}

StatusRequestPacket *status_request_packet_new() {
	StatusRequestPacket *packet = malloc(sizeof(StatusRequestPacket));
	uint8_t types_length = 0;
	PacketField *types = NULL;
    bool *retain_element = calloc(types_length, sizeof(bool));
	MCVarInt *packet_id = writeVarInt(0);
	PacketHeader wrapper = {
			.member_types = types,
			.members = types_length,
            .retain_types = retain_element,
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
    bool *retain_element = calloc(types_length, sizeof(bool));
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    MCVarInt *packet_id = writeVarInt(0x00);
    PacketHeader wrapper = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
            .direction = CLIENTBOUND,
            .state = STATUS,
            .packet_id = packet_id
    };
    packet->_header = wrapper;
    packet->response = response;
    return packet;
}

LoginStartPacket *login_start_packet_new(NetworkBuffer *player_name, bool offline_mode) {
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    if (offline_mode) {
        retain_element[2] = true;
        retain_element[3] = true;
        retain_element[4] = true;
        retain_element[6] = true;
    }
    MCVarInt *packet_id = writeVarInt(0x00);
    PacketHeader wrapper = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
            .direction = CLIENTBOUND,
            .state = STATUS,
            .packet_id = packet_id
    };
    packet->_header = wrapper;
    packet->player_name = player_name;
    packet->has_sig_data = !offline_mode;
    packet->has_player_uuid = !offline_mode;
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    MCVarInt *packet_id = writeVarInt(0x02);
    PacketHeader wrapper = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    MCVarInt *packet_id = writeVarInt(0x08);
    PacketHeader header = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    MCVarInt *packet_id = writeVarInt(0x15);
    PacketHeader header = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    MCVarInt *packet_id = writeVarInt(0x07);
    PacketHeader header = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    MCVarInt *packet_id = writeVarInt(0x00);
    PacketHeader header = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
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
        MCVarInt *dimension_count,
        NetworkBuffer *dimension_names,
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
    uint8_t types_length = 20;
    PacketField *types = malloc(types_length * sizeof(PacketField));
    PacketField typeArray[] = {
            PKT_UINT32,
            PKT_BOOL,
            PKT_UINT8,
            PKT_UINT8,
            PKT_VARINT,
            PKT_ARRAY,
            PKT_BYTEARRAY,
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
    bool *retain_element = calloc(types_length, sizeof(bool));
    if (!has_death_location) {
        retain_element[18] = true;
        retain_element[19] = true;
    }
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    MCVarInt *packet_id = writeVarInt(0x25);
    PacketHeader wrapper = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
            .direction = CLIENTBOUND,
            .state = STATUS,
            .packet_id = packet_id
    };
    packet->_header = wrapper;
    packet->entity_id = entity_id;
    packet->is_hardcore = is_hardcore;
    packet->gamemode = (uint8_t) gamemode;
    packet->previous_gamemode = (int8_t) previous_gamemode;
    packet->dimension_count = dimension_count;
    packet->dimension_names = dimension_names;
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
    bool *retain_element = calloc(types_length, sizeof(bool));
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    MCVarInt *packet_id = writeVarInt(0x19);
    PacketHeader wrapper = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    MCVarInt *packet_id = writeVarInt(0x39);
    PacketHeader header = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
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
    memcpy(types, typeArray, types_length * sizeof(PacketField));
    bool *retain_element = calloc(types_length, sizeof(bool));
    MCVarInt *packet_id = writeVarInt(0x39);
    PacketHeader header = {
            .member_types = types,
            .members = types_length,
            .retain_types = retain_element,
            .direction = CLIENTBOUND,
            .state = STATUS,
            .packet_id = packet_id
    };
    packet->_header = header;
    packet->no_of_recipes = no_of_recipes;
    packet->recipe = recipes;
    return packet;
}
