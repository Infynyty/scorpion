#include <stdlib.h>
#include "Packets.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"
#include "Logger.h"
#include "NBTParser.h"
#include "zlib.h"
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>

/** Utility **/

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

void packet_free(PacketHeader *packet) {
	void *ptr = packet + 1;
	for (int i = 0; i < packet->members; ++i) {
		PacketField m_type = packet->member_types[i];
		bool *optional_present = packet->optionals[i];
		if (optional_present != NULL && !*optional_present) {
			ptr = (void *) ptr;
			ptr += get_types_size(m_type);
			continue;
		}
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
//            case PKT_ARRAY: {
//                MCVarInt **varInt = (MCVarInt **) ptr;
//                uint32_t size = varint_decode(get_bytes(*varInt));
//                free(*varInt);
//                varInt++;
//                ptr = varInt;
//                for (int j = 0; j < size; ++j) {
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

static bool compression_enabled = false;
static int32_t compression_threshold = 0;

void set_compression(bool is_enabled) {
	compression_enabled = is_enabled;
}

void set_compression_threshold(int32_t threshold) {
	compression_threshold = threshold;
}

static bool encrpytion_enabled = false;

static EVP_CIPHER_CTX *ctx_encrypt;
static EVP_CIPHER_CTX *ctx_decrypt;

void init_encryption(NetworkBuffer *shared_secret) {
	encrpytion_enabled = true;
	ctx_encrypt = EVP_CIPHER_CTX_new();
	ctx_decrypt = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_init(ctx_encrypt);
	EVP_CIPHER_CTX_init(ctx_decrypt);

	EVP_EncryptInit_ex(ctx_encrypt, EVP_aes_128_cfb8(), NULL, shared_secret->bytes, shared_secret->bytes);
	EVP_DecryptInit_ex(ctx_decrypt, EVP_aes_128_cfb8(), NULL, shared_secret->bytes, shared_secret->bytes);
}

/** / Utility **/

/** Send **/

NetworkBuffer *packet_encode(PacketHeader *header) {
	NetworkBuffer *buffer = buffer_new();
	void *current_byte = header + 1;
	buffer_write_little_endian(buffer, header->packet_id->bytes, header->packet_id->size);
	for (int i = 0; i < header->members; ++i) {
		PacketField m_type = header->member_types[i];

		bool *is_optional = header->optionals[i];
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
					buffer_write_little_endian(buffer, varInt->bytes, varInt->size);
					break;
				}
				case PKT_VARLONG:
					//TODO: Fill in
					break;
				case PKT_BYTEARRAY:
				case PKT_UUID: {
					NetworkBuffer *uuid = *((NetworkBuffer **) current_byte);
					buffer_write(buffer, uuid->bytes, uuid->size);
					break;
				}
				case PKT_STRING: {
					NetworkBuffer *string = *((NetworkBuffer **) current_byte);
					MCVarInt *length = varint_encode(string->size);
					buffer_write_little_endian(buffer, length->bytes, length->size);
					buffer_write(buffer, string->bytes, string->size);


					free(length);
					break;
				}
				default:
					cmc_log(ERR, "Used unsupported type at packet_encode(): %d", m_type);
					exit(EXIT_FAILURE);
			}
		}
		current_byte = (void *) current_byte;
		current_byte += get_types_size(m_type);
	}
	return buffer;
}

NetworkBuffer *packet_compress(NetworkBuffer *packet) {
	NetworkBuffer *compressed = buffer_new();
	if (packet->size > compression_threshold) {
		MCVarInt *uncompressed_size = varint_encode(packet->size);

		char *temp = malloc(packet->size);
		uint64_t destLen = packet->size;
		compress((Bytef *) temp, (uLongf *) &destLen, packet->bytes, packet->size);

		MCVarInt *compressed_size = varint_encode(destLen);

		buffer_write_little_endian(compressed, compressed_size->bytes, compressed_size->size);
		buffer_write_little_endian(compressed, uncompressed_size->bytes, uncompressed_size->size);
		buffer_write_little_endian(compressed, packet->bytes, packet->size);

		free(temp);
	} else {
		MCVarInt *packet_length = varint_encode(packet->size);
		uint8_t data_length = 0;
		buffer_write_little_endian(compressed, packet_length->bytes, packet_length->size);
		buffer_write_little_endian(compressed, &data_length, sizeof(uint8_t));
		buffer_write_little_endian(compressed, packet->bytes, packet->size);
	}
	buffer_free(packet);
	return compressed;
}

NetworkBuffer *packet_encrypt(NetworkBuffer *packet) {
	NetworkBuffer *encrypted = buffer_new();
	unsigned char temp[packet->size + EVP_CIPHER_block_size(EVP_aes_128_cfb8()) - 1];
	int out_length;
	if (!EVP_EncryptUpdate(ctx_encrypt, temp, &out_length, packet->bytes, packet->size)) {
		cmc_log(ERR, "OpenSSL encryption error.");
		exit(EXIT_FAILURE);
	}
	buffer_write_little_endian(encrypted, temp, out_length);
	return encrypted;
}

void packet_send(PacketHeader *header) {
	NetworkBuffer *packet = packet_encode(header);
	if (compression_enabled && compression_threshold > 0) {
		packet = packet_compress(packet);
	} else {
		NetworkBuffer *length_prefixed = buffer_new();
		MCVarInt *packet_length = varint_encode(packet->size);
		buffer_write_little_endian(length_prefixed, packet_length->bytes, packet_length->size);
		buffer_write_little_endian(length_prefixed, packet->bytes, packet->size);
		buffer_free(packet);
		packet = length_prefixed;
	}
	if (encrpytion_enabled) {
		packet = packet_encrypt(packet);
	}
	send_wrapper(get_socket(), packet->bytes, packet->size);
}

/** Receive **/

void packet_decode(PacketHeader *header, NetworkBuffer *packet) {
	void *ptr = header + 1;
	for (int i = 0; i < header->members; ++i) {
		PacketField field = header->member_types[i];

		bool *is_optional = header->optionals[i];
		if (is_optional != NULL && *is_optional) continue;

		void *variable_pointer;
		size_t variable_size;
		switch (field) {
			case PKT_BOOL:
			case PKT_BYTE:
			case PKT_UINT8: {
				uint8_t uint8 = buffer_read(uint8_t, packet);
				variable_pointer = &uint8;
				variable_size = sizeof(uint8_t);
				break;
			}
			case PKT_UINT16: {
				uint16_t uint16 = buffer_read(uint16_t, packet);
				variable_pointer = &uint16;
				variable_size = sizeof(uint16_t);
				break;
			}
			case PKT_UINT32: {
				uint32_t uint32 = buffer_read(uint32_t, packet);
				variable_pointer = &uint32;
				variable_size = sizeof(uint32_t);
				break;
			}
			case PKT_UINT64: {
				uint64_t uint64 = buffer_read(uint64_t, packet);
				variable_pointer = &uint64;
				variable_size = sizeof(uint64_t);
				break;
			}
			case PKT_FLOAT: {
				float number = buffer_read(float, packet);
				variable_pointer = &number;
				variable_size = sizeof(float);
				break;
			}
			case PKT_DOUBLE: {
				double number = buffer_read(double, packet);
				variable_pointer = &number;
				variable_size = sizeof(double);
				break;
			}
			case PKT_VARINT: {
				MCVarInt *var_int = varint_encode(buffer_read_varint(packet));
				variable_pointer = &var_int;
				variable_size = sizeof(MCVarInt *);
				break;
			}
			case PKT_UUID: {
				NetworkBuffer *uuid = buffer_new();
				buffer_poll(packet, 2 * sizeof(uint64_t), uuid);
				variable_pointer = &uuid;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_BYTEARRAY: {
				NetworkBuffer *bytes = buffer_new();
				buffer_read_array(packet, bytes);
				variable_pointer = &bytes;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_STRING_ARRAY: {
				NetworkBuffer *strings = buffer_new();
				uint32_t length = buffer_read_varint(packet);
				for (int j = 0; j < length; ++j) {
					buffer_read_array(packet, strings);
				}
				variable_pointer = &strings;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_STRING: {
				NetworkBuffer *string = buffer_new();
				buffer_read_array(packet, string);
				variable_pointer = &string;
				variable_size = sizeof(NetworkBuffer *);
				break;
			}
			case PKT_NBTTAG: {
				cmc_log(ERR, "Tried receiving unhandled packet field %d", field);
				exit(EXIT_FAILURE);
				NetworkBuffer *nbt = buffer_new();
				consume_nbt_data(get_socket());
				variable_pointer = &nbt;
				variable_size = sizeof(NetworkBuffer *);
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
		memcpy(ptr, variable_pointer, variable_size);
		ptr += variable_size;
	}
}

uint8_t receive_byte() {
	unsigned char byte;
	receive_wrapper(get_socket(), &byte, sizeof(byte));
	if (encrpytion_enabled) {
		int32_t outlen;
		unsigned char out;
		if (!EVP_DecryptUpdate(ctx_decrypt, &out, &outlen, &byte, sizeof(byte))) {
			cmc_log(ERR, "OpenSSL decryption error");
			exit(EXIT_FAILURE);
		}
		byte = out;
	}
	return byte;
}

NetworkBuffer *receive_bytes(uint64_t size) {
	NetworkBuffer *bytes = buffer_new();
	for (int i = 0; i < size; i++) {
		uint8_t byte = receive_byte();
		buffer_write_little_endian(bytes, &byte, sizeof(byte));
	}
	return bytes;
}

int32_t receive_varint() {
	unsigned char current_byte;
	int result = 0;
	const int CONTINUE_BIT = 0b10000000;
	const int SEGMENT_BITS = 0b01111111;
	for (int i = 0; i < 5; ++i) {
		current_byte = receive_byte();
		result += (current_byte & SEGMENT_BITS) << (8 * i - i);
		if ((current_byte & CONTINUE_BIT) != (CONTINUE_BIT)) {
			break;
		}
	}
	return result;
}

GenericPacket *packet_receive() {
	GenericPacket *packet = malloc(sizeof(GenericPacket));
	if (!compression_enabled) {
		uint32_t length = receive_varint();
		uint32_t packet_id = receive_varint();

		packet->is_compressed = false;
		packet->uncompressed_length = length;
		packet->packet_id = packet_id;
		packet->data = receive_bytes(length - varint_encode(packet_id)->size);
	} else {
		int32_t compressed_length = receive_varint();
		int32_t uncompressed_length = receive_varint();
		NetworkBuffer *compressed_data = receive_bytes(compressed_length);

		if (uncompressed_length != 0) {
			char uncompressed_data_temp[uncompressed_length];
			uncompress((Bytef *) uncompressed_data_temp,
			           (uLongf *) &uncompressed_length,
			           (Bytef *) compressed_data->bytes,
			           (uLong) compressed_length);
			NetworkBuffer *uncompressed_data = buffer_new();
			buffer_write_little_endian(
					uncompressed_data,
					uncompressed_data_temp,
					uncompressed_length
			);
			uint32_t packet_id = buffer_read_varint(uncompressed_data);

			packet->is_compressed = true;
			packet->compressed_length = compressed_length;
			packet->uncompressed_length = uncompressed_length - 1;
			packet->packet_id = packet_id;
			packet->data = uncompressed_data;
		} else {
			uint32_t packet_id = buffer_read_varint(compressed_data);

			packet->is_compressed = false;
			packet->compressed_length = -1;
			packet->uncompressed_length = compressed_length - 1;
			packet->packet_id = packet_id;
			packet->data = compressed_data;
		}
	}
	return packet;
}

//TODO: Two constructors for every packet:
// Empty: Used to receive a packet over the network. Optional elements? Optionals Array in packet header, where each optional
// contains a bool ptr.

void packet_generate_header(
		PacketHeader *header,
		PacketField type_array[],
		uint8_t types_size,
		uint8_t packet_id,
		PacketDirection direction,
		ConnectionState state
) {
	PacketField *types = malloc(types_size * sizeof(PacketField));
	bool **optionals = calloc(types_size, sizeof(bool *));
	memcpy(types, type_array, types_size * sizeof(PacketField));
	MCVarInt *packet_id_enc = varint_encode(packet_id);

	header->member_types = types;
	header->members = types_size;
	header->optionals = optionals;
	header->packet_id = packet_id_enc;
	header->direction = direction;
	header->state = state;
}

HandshakePacket *handshake_pkt_new(NetworkBuffer *address, unsigned short port, HandshakeNextState state) {
	HandshakePacket *packet = malloc(sizeof(HandshakePacket));
	PacketHeader header = {};
	PacketField fields[] = {
			PKT_VARINT,
			PKT_STRING,
			PKT_UINT16,
			PKT_VARINT
	};
	packet_generate_header(&header, fields, 4, 0x00, SERVERBOUND, HANDSHAKE);
	packet->_header = header;
	packet->protocol_version = varint_encode(760);
	packet->address = address;
	packet->port = port;
	packet->next_state = varint_encode(state);
	return packet;
}

StatusRequestPacket *status_request_packet_new() {
	StatusRequestPacket *packet = malloc(sizeof(StatusRequestPacket));
	PacketHeader header = {};
	packet_generate_header(&header, NULL, 0, 0x00, SERVERBOUND, STATUS);
	packet->_header = header;
	return packet;
}

StatusResponsePacket *status_response_packet_new(NetworkBuffer *response) {
	StatusResponsePacket *packet = malloc(sizeof(StatusResponsePacket));
	PacketHeader header = {};
	PacketField fields[] = {PKT_STRING};
	packet_generate_header(&header, fields, 1, 0x00, CLIENTBOUND, STATUS);
	packet->_header = header;
	packet->response = response;
	return packet;
}

LoginStartPacket *login_start_packet_new(
		NetworkBuffer *player_name,
		bool has_sig_data,
		bool has_player_uuid,
		NetworkBuffer *uuid
) {
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
	bool **optionals = calloc(types_length, sizeof(bool *));
	if (!has_sig_data) {
		optionals[2] = &packet->has_sig_data;
		optionals[3] = &packet->has_sig_data;
		optionals[4] = &packet->has_sig_data;
	}
	if (!has_player_uuid) {
		optionals[6] = &packet->has_player_uuid;
	}
	MCVarInt *packet_id = varint_encode(0x00);
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
	packet->uuid = uuid;
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));

	optionals[2] = &packet->has_verify_token;
	optionals[3] = &packet->_has_no_verify_token;
	optionals[4] = &packet->_has_no_verify_token;

	MCVarInt *packet_id = varint_encode(0x01);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x01);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x02);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x08);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x15);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x07);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x00);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	MCVarInt *packet_id = varint_encode(0x25);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	MCVarInt *packet_id = varint_encode(0x19);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x39);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x39);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x0b);
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
	memcpy(types, typeArray, types_length * sizeof(PacketField));
	bool **optionals = calloc(types_length, sizeof(bool *));
	MCVarInt *packet_id = varint_encode(0x31);
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
