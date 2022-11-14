/**
 * Contains all methods, structs and enums that are helpful to create, manage, send and free packets.
 */

#ifndef CMC_PACKETS_H
#define CMC_PACKETS_H


#include <stdbool.h>
#include "MCVarInt.h"
#include "NetworkBuffer.h"
#include "ConnectionState.h"

typedef enum PacketField {
	PKT_BOOL, PKT_BYTE, PKT_UINT8, PKT_UINT16, PKT_UINT32, PKT_UINT64, PKT_FLOAT,
	PKT_DOUBLE, PKT_STRING, PKT_CHAT, PKT_IDENTIFIER, PKT_VARINT, PKT_VARLONG,
	PKT_ENTITYMETA, PKT_SLOT, PKT_NBTTAG, PKT_OPTIONAL, PKT_ARRAY, PKT_ENUM,
	PKT_BYTEARRAY, PKT_UUID
} PacketField;

typedef struct {
	uint8_t members;
	PacketField *member_types;
	bool **optionals;
	ConnectionState state;
	PacketDirection direction;
	MCVarInt *packet_id;
} PacketHeader;

void packet_send(PacketHeader *packet, SocketWrapper *socket);

void packet_receive(PacketHeader *header);

void packet_free(PacketHeader *packet);

/** State: Handshake **/

/** Serverbound **/

typedef enum HandshakeNextState {
	HANDSHAKE_STATUS = 1, HANDSHAKE_LOGIN = 2
} HandshakeNextState;

typedef struct __attribute__((__packed__)) HandshakePacket {
	PacketHeader _header;
	MCVarInt *protocol_version;
	NetworkBuffer *address;
	unsigned short port;
	MCVarInt *next_state;
} HandshakePacket;

HandshakePacket *handshake_pkt_new(NetworkBuffer *address, unsigned short port, HandshakeNextState state);


/** State: Status **/

/** Serverbound **/

typedef struct StatusRequestPacket {
	PacketHeader _header;
} StatusRequestPacket;

StatusRequestPacket *status_request_packet_new();

typedef struct StatusResponsePacket {
	PacketHeader _header;
	NetworkBuffer *response;
} StatusResponsePacket;

StatusResponsePacket *status_response_packet_new(NetworkBuffer *response);

struct PingRequestPacket {
	MCVarInt *packetID;
	uint64_t *payload;
};

/** Clientbound **/



/** State: Login **/

/** Serverbound **/

typedef struct __attribute__((__packed__)) LoginStartPacket {
	PacketHeader _header;
	NetworkBuffer *player_name;
	bool has_sig_data;
	uint64_t timestamp;
	NetworkBuffer *public_key;
	NetworkBuffer *signature;
	bool has_player_uuid;
	NetworkBuffer *uuid;
} LoginStartPacket;

LoginStartPacket *login_start_packet_new(NetworkBuffer *player_name, bool offline_mode);

/** Clientbound **/

typedef struct __attribute__((__packed__)) LoginSuccessPacket {
	PacketHeader _header;
	NetworkBuffer *uuid;
	NetworkBuffer *username;
	MCVarInt *number_of_properties;
	NetworkBuffer *properties;
} LoginSuccessPacket;

LoginSuccessPacket *login_success_packet_new(
		NetworkBuffer *uuid,
		NetworkBuffer *username,
		MCVarInt *number_of_properties,
		NetworkBuffer *properties
);

/** State: Play **/

/** Serverbound **/

typedef struct __attribute__((__packed__)) ConfirmTeleportationPacket {
	PacketHeader _header;
	MCVarInt *teleport_id;
} ConfirmTeleportationPacket;

ConfirmTeleportationPacket *confirm_teleportation_packet_new(MCVarInt *teleport_id);

typedef enum ChatMode {
	ENABLED = 0, COMMANDS_ONLY = 1, HIDDEN = 2
} ChatMode;

typedef enum DisplayedSkinParts {
	CAPE = 0x01,
	JACKET = 0x02,
	LEFT_SLEEVE = 0x04,
	RIGHT_SLEEVE = 0x08,
	LEFT_PANTS = 0x10,
	RIGHT_PANTS = 0x20,
	HAT = 0x40
} DisplayedSkinParts;

typedef enum MainHand {
	MAINHAND_LEFT = 0, MAINHAND_RIGHT = 1
} MainHand;

typedef struct __attribute__((__packed__)) ClientInformationPacket {
	PacketHeader _header;
	NetworkBuffer *locale;
	uint8_t view_distance;
	MCVarInt *chat_mode;
	bool chat_colors;
	uint8_t displayed_skin_parts;
	MCVarInt *main_hand;
	bool enable_text_filtering;
	bool allow_server_listings;
} ClientInformationPacket;

ClientInformationPacket *client_info_packet_new(
		NetworkBuffer *locale,
		uint8_t view_distance,
		MCVarInt *chat_mode,
		bool chat_colors,
		uint8_t displayed_skin_parts,
		MCVarInt *main_hand,
		bool enable_text_filtering,
		bool allow_server_listings
);

typedef struct __attribute__((__packed__)) SetPlayerPosAndRotPacket {
	PacketHeader _header;
	double x;
	double y;
	double z;
	float yaw;
	float pitch;
	bool on_ground;
} SetPlayerPosAndRotPacket;

SetPlayerPosAndRotPacket *set_player_pos_and_rot_packet_new(
		double x,
		double y,
		double z,
		float yaw,
		float pitch,
		bool on_ground
);

typedef enum ClientCommandAction {
	CLIENT_ACTION_RESPAWN, CLIENT_ACTION_REQUEST_STATS
} ClientCommandAction;

typedef struct __attribute__((__packed__)) ClientCommandPacket {
	PacketHeader _header;
	MCVarInt *action;
} ClientCommandPacket;

ClientCommandPacket *client_command_packet_new(MCVarInt *action);


/** Clientbound **/

typedef enum Gamemode {
	NONE = -1, SURVIVAL = 0, CREATIVE = 1, ADVENTURE = 2, SPECTATOR = 3
} Gamemode;

typedef struct __attribute__((__packed__)) LoginPlayPacket {
	PacketHeader _header;
	uint32_t entity_id;
	bool is_hardcore;
	uint8_t gamemode;
	uint8_t previous_gamemode;
	MCVarInt *dimension_count;
	NetworkBuffer *dimension_names;
	NetworkBuffer *registry_codec;
	NetworkBuffer *spawn_dimension_name;
	NetworkBuffer *spawn_dimension_type;
	uint64_t hashed_seed;
	MCVarInt *_max_players;                  // ignored by the Notchian client
	MCVarInt *view_distance;
	MCVarInt *simulation_distance;
	bool reduced_debug_info;
	bool enable_respawn_screen;
	bool is_debug;
	bool is_flat;
	bool has_death_location;
	NetworkBuffer *death_dimension_name;
	uint64_t death_location;
} LoginPlayPacket;

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
);

typedef struct __attribute__((__packed__)) DisconnectPlayPacket {
	PacketHeader _header;
	NetworkBuffer *reason;
} DisconnectPlayPacket;

DisconnectPlayPacket *disconnect_play_packet_new(NetworkBuffer *reason);

typedef enum PositionFlag {
	X_RELATIVE = 0x01, Y_RELATIVE = 0x02, Z_RELATIVE = 0x04, Y_ROT_RELATIVE = 0x08, X_ROT_RELATIVE = 0x10
} PositionFlag;

typedef struct __attribute__((__packed__)) SynchronizePlayerPositionPacket {
	PacketHeader _header;
	double x;
	double y;
	double z;
	float yaw;
	float pitch;
	uint8_t flags;
	MCVarInt *teleport_id;
	bool dismount_vehicle;
} SynchronizePlayerPositionPacket;

SynchronizePlayerPositionPacket *synchronize_player_position_packet_new(
		double x,
		double y,
		double z,
		float yaw,
		float pitch,
		uint8_t flags,
		MCVarInt *teleport_id,
		bool dismount_vehicle
);

typedef struct __attribute__((__packed__)) UpdateRecipesPacket {
	PacketHeader _header;
	MCVarInt *no_of_recipes;
	NetworkBuffer *recipe;
} UpdateRecipesPacket;

UpdateRecipesPacket *update_recipes_packet_new(MCVarInt *no_of_recipes, NetworkBuffer *recipes);

typedef enum Difficulty {
	PEACEFUL = 0, EASY = 1, NORMAL = 2, HARD = 3
} Difficulty;

typedef struct __attribute__((__packed__)) ChangeDifficultyPacket {
	PacketHeader _header;
	uint8_t difficulty;
	bool difficulty_locked;
} ChangeDifficultyPacket;

ChangeDifficultyPacket *change_difficulty_packet_new(uint8_t difficulty, bool difficulty_locked);

typedef enum PlayerAbilitiesFlags {
	INVULNERABLE = 0x01, FLYING = 0x02, ALLOW_FLYING = 0x04, CREATIVE_MODE = 0x08
} PlayerAbilitiesFlags;

typedef struct __attribute__((__packed__)) PlayerAbilitiesCBPacket {
	PacketHeader _header;
	uint8_t flags;
	float flying_speed;
	float fov_modifier;
} PlayerAbilitiesCBPacket;

PlayerAbilitiesCBPacket *player_abilities_cb_packet_new(uint8_t flags, float flying_speed, float fov_modifier);

#endif //CMC_PACKETS_H


