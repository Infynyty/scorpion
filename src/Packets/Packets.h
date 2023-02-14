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
	PKT_BYTEARRAY, PKT_UUID, PKT_STRING_ARRAY, PKT_LOGIN_PROPERTIES, PKT_PREV_MSG_ARRAY
} PacketField;

typedef struct GenericPacket {
	bool is_compressed;
	uint8_t compressed_length;
	uint8_t uncompressed_length;
	uint8_t compressed_packet_id;
	uint8_t packet_id;
	NetworkBuffer *data;
} GenericPacket;

void generic_packet_free(GenericPacket *packet);

typedef struct {
	uint8_t members;
	PacketField *member_types;
	bool **optionals;           // indicates whether optional fields are present
	ConnectionState state;
	PacketDirection direction;
	MCVarInt *packet_id;
} PacketHeader;

/** Send **/

/**
 * Sends a packet struct to the server. This method will encode the bytes into the required format and compress as well
 * as encrypt the packet if needed.
 * @param header The header of the packet struct that will be sent.
 *
 */
void packet_send(PacketHeader **header);

GenericPacket *packet_receive();

void packet_decode(PacketHeader **header, NetworkBuffer *generic_packet);

void packet_free(PacketHeader **packet);

void set_compression_threshold(int32_t threshold);

void init_encryption(NetworkBuffer *shared_secret);

/** State: Handshake **/

/** Serverbound **/

typedef enum HandshakeNextState {
	HANDSHAKE_STATUS = 1, HANDSHAKE_LOGIN = 2
} HandshakeNextState;

typedef struct __attribute__((__packed__)) HandshakePacket {
	PacketHeader *_header;
	MCVarInt *protocol_version;
	NetworkBuffer *address;
	unsigned short port;
	MCVarInt *next_state;
} HandshakePacket;

PacketHeader * handshake_pkt_header();


/** State: Status **/

/** Serverbound **/

typedef struct StatusRequestPacket {
    PacketHeader *_header;
} StatusRequestPacket;

PacketHeader * status_request_packet_new();

typedef struct StatusResponsePacket {
    PacketHeader *_header;
	NetworkBuffer *response;
} StatusResponsePacket;

PacketHeader * status_response_packet_new();

struct PingRequestPacket {
    PacketHeader *_header;
    MCVarInt *packetID;
	uint64_t *payload;
};

/** Clientbound **/



/** State: Login **/

/** Serverbound **/

typedef struct __attribute__((__packed__)) LoginStartPacket {
	PacketHeader *_header;
	NetworkBuffer *player_name;
	bool has_player_uuid;
	NetworkBuffer *uuid;
} LoginStartPacket;

PacketHeader *login_start_packet_header();

typedef struct __attribute__((__packed__)) EncryptionResponsePacket {
    PacketHeader *_header;
	NetworkBuffer *shared_secret;
	NetworkBuffer *verify_token;
} EncryptionResponsePacket;

PacketHeader *encryption_response_packet_new();

/** Clientbound **/

typedef struct __attribute__((__packed__)) DisconnectLoginPacket {
    PacketHeader *_header;
	NetworkBuffer *reason;
} DisconnectLoginPacket;

PacketHeader *disconnect_login_packet_new();

typedef struct __attribute__((__packed__)) EncryptionRequestPacket {
    PacketHeader *_header;
	NetworkBuffer *server_id;
	NetworkBuffer *public_key;
	NetworkBuffer *verify_token;
} EncryptionRequestPacket;

PacketHeader * encryption_request_packet_new();

typedef struct __attribute__((__packed__)) SetCompressionPacket {
    PacketHeader *_header;
	MCVarInt *threshold;
} SetCompressionPacket;

PacketHeader * set_compression_packet_new();


typedef struct __attribute__((__packed__)) LoginSuccessPacket {
    PacketHeader *_header;
	NetworkBuffer *uuid;
	NetworkBuffer *username;
	MCVarInt *number_of_properties;
	NetworkBuffer *properties;
} LoginSuccessPacket;

PacketHeader * login_success_packet_new();

/** State: Play **/

/** Serverbound **/

typedef struct __attribute__((__packed__)) ConfirmTeleportationPacket {
    PacketHeader *_header;
	MCVarInt *teleport_id;
} ConfirmTeleportationPacket;

PacketHeader *confirm_teleportation_packet_new();

typedef enum ChatMode {
	CHAT_ENABLED = 0, CHAT_COMMANDS_ONLY = 1, CHAT_HIDDEN = 2
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
    PacketHeader *_header;
	NetworkBuffer *locale;
	uint8_t view_distance;
	MCVarInt *chat_mode;
	bool chat_colors;
	uint8_t displayed_skin_parts;
	MCVarInt *main_hand;
	bool enable_text_filtering;
	bool allow_server_listings;
} ClientInformationPacket;

PacketHeader * client_info_packet_new();

typedef struct __attribute__((__packed__)) SetPlayerPosAndRotPacket {
    PacketHeader *_header;
	double x;
	double y;
	double z;
	float yaw;
	float pitch;
	bool on_ground;
} SetPlayerPosAndRotPacket;

PacketHeader * set_player_pos_and_rot_packet_new();

typedef enum ClientCommandAction {
	CLIENT_ACTION_RESPAWN = 0, CLIENT_ACTION_REQUEST_STATS = 1
} ClientCommandAction;

typedef struct __attribute__((__packed__)) ClientCommandPacket {
    PacketHeader *_header;
	MCVarInt *action;
} ClientCommandPacket;

PacketHeader *client_command_packet_new();


/** Clientbound **/

typedef enum Gamemode {
	NONE = -1, SURVIVAL = 0, CREATIVE = 1, ADVENTURE = 2, SPECTATOR = 3
} Gamemode;

typedef struct __attribute__((__packed__)) LoginPlayPacket {
    PacketHeader *_header;
	uint32_t entity_id;
	bool is_hardcore;
	uint8_t gamemode;
	uint8_t previous_gamemode;
	NetworkBuffer *dimensions;
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

PacketHeader * login_play_packet_new(bool *hasDeathLocation);

typedef struct __attribute__((__packed__)) DisconnectPlayPacket {
    PacketHeader *_header;
	NetworkBuffer *reason;
} DisconnectPlayPacket;

PacketHeader * disconnect_play_packet_new();

typedef enum PositionFlag {
	X_RELATIVE = 0x01, Y_RELATIVE = 0x02, Z_RELATIVE = 0x04, Y_ROT_RELATIVE = 0x08, X_ROT_RELATIVE = 0x10
} PositionFlag;

typedef struct __attribute__((__packed__)) SynchronizePlayerPositionPacket {
    PacketHeader *_header;
	double x;
	double y;
	double z;
	float yaw;
	float pitch;
	uint8_t flags;
	MCVarInt *teleport_id;
	bool dismount_vehicle;
} SynchronizePlayerPositionPacket;

PacketHeader * synchronize_player_position_packet_new();

typedef struct __attribute__((__packed__)) UpdateRecipesPacket {
    PacketHeader *_header;
	MCVarInt *no_of_recipes;
	NetworkBuffer *recipe;
} UpdateRecipesPacket;

PacketHeader * update_recipes_packet_new();

typedef enum Difficulty {
	PEACEFUL = 0, EASY = 1, NORMAL = 2, HARD = 3
} Difficulty;

typedef struct __attribute__((__packed__)) ChangeDifficultyPacket {
    PacketHeader *_header;
	uint8_t difficulty;
	bool difficulty_locked;
} ChangeDifficultyPacket;

PacketHeader * change_difficulty_packet_new();

typedef enum PlayerAbilitiesFlags {
	INVULNERABLE = 0x01, FLYING = 0x02, ALLOW_FLYING = 0x04, CREATIVE_MODE = 0x08
} PlayerAbilitiesFlags;

typedef struct __attribute__((__packed__)) PlayerAbilitiesCBPacket {
    PacketHeader *_header;
	uint8_t flags;
	float flying_speed;
	float fov_modifier;
} PlayerAbilitiesCBPacket;

PacketHeader * player_abilities_cb_packet_new();

typedef struct __attribute__((__packed__)) KeepAliveClientboundPacket {
    PacketHeader *_header;
    int64_t payload;
} KeepAliveClientboundPacket;

PacketHeader * keep_alive_clientbound_packet_new();

typedef struct __attribute__((__packed__)) KeepAliveServerboundPacket {
    PacketHeader *_header;
    int64_t payload;
} KeepAliveServerboundPacket;

PacketHeader * keep_alive_serverbound_new();

typedef struct __attribute__((__packed__)) PlayerChatMessagePacket {
    PacketHeader *_header;
    NetworkBuffer *sender;
    MCVarInt *index;
    bool has_message_signature;
    NetworkBuffer *signature;
    NetworkBuffer *message;
    uint64_t timestamp;
    uint64_t salt;
    MCVarInt *no_of_prev_messages;
    NetworkBuffer *prev_messages;
    bool has_unsigned_content;
    NetworkBuffer *unsigned_content;
    MCVarInt *filter_type;
    MCVarInt *filter_type_bits;
    MCVarInt *chat_type;
    NetworkBuffer *network_name;
    bool network_target_name_present;
} PlayerChatMessagePacket;

PacketHeader *player_chat_message_header(bool *has_signature, bool *has_unsigned_content, bool *has_target_network);

#endif //CMC_PACKETS_H


