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
#include <arpa/inet.h>
#include <pthread.h>

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
        case PKT_INT32:
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

void generic_packet_free(GenericPacket *packet) {
    buffer_free(packet->data);
    free(packet);
}

#pragma GCC push_options
#pragma GCC optimize("O0")
void packet_free(PacketHeader **packet) {
    if (packet == NULL || *packet == NULL) return;
    void *ptr = ((PacketHeader **) (packet)) + 1;
    for (int i = 0; i < (*packet)->members; ++i) {
        PacketField m_type = (*packet)->member_types[i];
        bool *optional_present = (*packet)->optionals[i];
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
            case PKT_INT32:
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
            case PKT_LOGIN_PROPERTIES:
            case PKT_UUID:
            case PKT_NBTTAG:
            case PKT_CHAT:
            case PKT_STRING: {
                NetworkBuffer **string = (NetworkBuffer **) ptr;
                buffer_free(*string);
                string++;
                ptr = string;
                break;
            }
            case PKT_SKIP:
                break;
            case PKT_VARLONG:
            case PKT_IDENTIFIER:
            case PKT_ENTITYMETA:
            case PKT_SLOT:
            case PKT_OPTIONAL:
            case PKT_ENUM:
            default:
                sc_log(ERR, "Used unsupported type at packet_free(): %d", m_type);
                exit(EXIT_FAILURE);
        }
    }
    ptr = NULL;
    int32_t id = varint_decode((*(packet))->packet_id->bytes);
    if ((*packet)->optionals != NULL) free((*packet)->optionals);
    if ((*packet)->member_types != NULL) free((*packet)->member_types);
    if ((*packet)->packet_id != NULL) free((*packet)->packet_id);
    free((*packet));
    (*packet) = NULL;
}
#pragma GCC pop_options

static bool compression_enabled = false;
static int32_t compression_threshold = 0;

void set_compression_threshold(int32_t threshold) {
    compression_threshold = threshold;
    if (threshold > 0) {
        compression_enabled = true;
    }
}


static bool encryption_enabled = false;

static EVP_CIPHER_CTX *ctx_encrypt;
static EVP_CIPHER_CTX *ctx_decrypt;

void init_encryption(NetworkBuffer *shared_secret) {
    sc_log(INFO, "Enabled encryption");
    encryption_enabled = true;
    ctx_encrypt = EVP_CIPHER_CTX_new();
    ctx_decrypt = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx_encrypt);
    EVP_CIPHER_CTX_init(ctx_decrypt);

    EVP_EncryptInit_ex(ctx_encrypt, EVP_aes_128_cfb8(), NULL, shared_secret->bytes, shared_secret->bytes);
    EVP_DecryptInit_ex(ctx_decrypt, EVP_aes_128_cfb8(), NULL, shared_secret->bytes, shared_secret->bytes);
}

/** / Utility **/

/** Send **/

NetworkBuffer *packet_encode(PacketHeader **header) {
    NetworkBuffer *buffer = buffer_new();
    void *current_byte = ((PacketHeader **) (header)) + 1;
    buffer_write(buffer, (*header)->packet_id->bytes, (*header)->packet_id->size);
    for (int i = 0; i < (*header)->members; ++i) {
        PacketField m_type = (*header)->member_types[i];

        bool *is_optional = (*header)->optionals[i];
        if (is_optional == NULL || *is_optional) {
            switch (m_type) {
                case PKT_BOOL:
                case PKT_UINT8: {
                    buffer_write(buffer, current_byte, get_types_size(m_type));
                    break;
                }
                case PKT_UINT16: {
                    uint16_t num = htons(*((u_int16_t *)current_byte));
                    buffer_write(buffer, &num, get_types_size(m_type));
                    break;
                }
                case PKT_UINT32: {
                    uint32_t num = htonl(*((u_int32_t *)current_byte));
                    buffer_write(buffer, &num, get_types_size(m_type));
                    break;
                }
                case PKT_INT32: {
                    int32_t num;
#ifdef __APPLE__
                    num = htonl(*((u_int32_t *)current_byte));
#endif
#ifdef linux
                    num = htobe32(*((u_int32_t *)current_byte));
#endif
                    buffer_write(buffer, &num, get_types_size(m_type));
                    break;
                }
                case PKT_UINT64: {
                    int64_t num;
#ifdef __APPLE__
                    num = htonll(*((u_int64_t *)current_byte));
#endif
#if linux
                    num = htobe64(*((u_int64_t *)current_byte));
#endif
                    buffer_write(buffer, &num, get_types_size(m_type));
                    break;
                }
                case PKT_FLOAT: {
                    uint32_t num;
                    memmove(&num, current_byte, sizeof(float));
                    num = htonl(num);
                    buffer_write(buffer, &num, sizeof(float ));
                    break;
                }
                case PKT_DOUBLE: {
                    uint64_t num;
                    memmove(&num, current_byte, sizeof(double ));
#ifdef __APPLE__
                    num = htonll(num);
#endif
#ifdef linux
                    num = htobe64(num);
#endif
                    buffer_write(buffer, &num, sizeof(double ));
                    break;
                }
                case PKT_VARINT: {
                    MCVarInt *varInt = (*(MCVarInt **) (current_byte));
                    buffer_write(buffer, varInt->bytes, varInt->size);
                    break;
                }
                case PKT_VARLONG:
                    //TODO: Fill in
                    break;
                case PKT_UUID: {
                    NetworkBuffer *uuid = *((NetworkBuffer **) current_byte);
#ifdef __LITTLE_ENDIAN__
                    buffer_swap_endianness(uuid);
#endif
                    buffer_write(buffer, uuid->bytes, uuid->size);
                    break;
                }
                case PKT_BYTEARRAY:
                case PKT_STRING: {
                    NetworkBuffer *string = *((NetworkBuffer **) current_byte);
                    MCVarInt *length = varint_encode(string->size);
                    buffer_write(buffer, length->bytes, length->size);
                    buffer_write(buffer, string->bytes, string->size);
                    free(length);
                    break;
                }
                default:
                    sc_log(ERR, "Used unsupported type at packet_encode(): %d", m_type);
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

        MCVarInt *compressed_size = varint_encode(destLen + uncompressed_size->size);

        buffer_write(compressed, compressed_size->bytes, compressed_size->size);
        buffer_write(compressed, uncompressed_size->bytes, uncompressed_size->size);
        buffer_write(compressed, packet->bytes, packet->size);

        free(temp);
    } else {
        MCVarInt *packet_length = varint_encode(packet->size + 1);
        uint8_t data_length = 0;
        buffer_write(compressed, packet_length->bytes, packet_length->size);
        buffer_write(compressed, &data_length, sizeof(uint8_t));
        buffer_write(compressed, packet->bytes, packet->size);
        free(packet_length);
    }
    buffer_free(packet);
    return compressed;
}


NetworkBuffer *packet_encrypt(NetworkBuffer *packet) {
    NetworkBuffer *encrypted = buffer_new();
    unsigned char temp[packet->size + EVP_CIPHER_block_size(EVP_aes_128_cfb8()) - 1];
    int out_length;
    if (!EVP_EncryptUpdate(ctx_encrypt, temp, &out_length, packet->bytes, packet->size)) {
        sc_log(ERR, "OpenSSL encryption error.");
        exit(EXIT_FAILURE);
    }
    buffer_write(encrypted, temp, out_length);
    buffer_free(packet);
    return encrypted;
}


void packet_send(PacketHeader **header) {
    NetworkBuffer *packet = packet_encode(header);
    if (compression_enabled && compression_threshold > 0) {
        packet = packet_compress(packet);
    } else {
        NetworkBuffer *length_prefixed = buffer_new();
        MCVarInt *packet_length = varint_encode(packet->size);
        buffer_write(length_prefixed, packet_length->bytes, packet_length->size);
        buffer_write(length_prefixed, packet->bytes, packet->size);
        buffer_free(packet);
        free(packet_length);
        packet = length_prefixed;
    }
    if (encryption_enabled) {
        packet = packet_encrypt(packet);
    }
    send_wrapper(get_socket(), packet->bytes, packet->size);
    buffer_free(packet);
}


/** Receive **/
#pragma GCC push_options
#pragma GCC optimize("O0")
void packet_decode(PacketHeader **header, NetworkBuffer *generic_packet) {
    void *ptr = header + 1;
    for (int i = 0; i < (*header)->members; ++i) {
        PacketField field = (*header)->member_types[i];
        bool *is_optional = (*header)->optionals[i];
        if (is_optional != NULL && !(*is_optional)) {
            ptr = (void *) ptr;
            ptr += get_types_size(field);
            continue;
        }

        void *variable_pointer;
        size_t variable_size;
        switch (field) {
            case PKT_BOOL:
            case PKT_BYTE:
            case PKT_UINT8: {
                uint8_t uint8 = buffer_read(uint8_t, generic_packet);
                variable_pointer = &uint8;
                variable_size = sizeof(uint8_t);
                break;
            }
            case PKT_UINT16: {
                uint16_t uint16 = buffer_read(uint16_t, generic_packet);
                uint16 = ntohs(uint16);
                variable_pointer = &uint16;
                variable_size = sizeof(uint16_t);
                break;
            }
            case PKT_UINT32: {
                uint32_t uint32 = buffer_read(uint32_t, generic_packet);
                uint32 = ntohl(uint32);
                variable_pointer = &uint32;
                variable_size = sizeof(uint32_t);
                break;
            }
            case PKT_INT32: {
                int32_t int32 = buffer_read(int32_t, generic_packet);
                int32 = (int32_t) ntohl(int32);
                variable_pointer = &int32;
                variable_size = sizeof(int32_t);
                break;
            }
            case PKT_UINT64: {
                uint64_t uint64 = buffer_read(uint64_t, generic_packet);
#ifdef __APPLE__
                uint64 = ntohll(uint64);
#endif
#ifdef linux
                uint64 = be64toh(uint64);
#endif
                variable_pointer = &uint64;
                variable_size = sizeof(uint64_t);
                break;
            }
            case PKT_FLOAT: {
                NetworkBuffer *swap = buffer_new();
                buffer_move(generic_packet, sizeof(float), swap);
                buffer_swap_endianness(swap);
                float number;
                memmove(&number, swap->bytes, swap->size);
                buffer_free(swap);
                variable_pointer = &number;
                variable_size = sizeof(float);
                break;
            }
            case PKT_DOUBLE: {
                NetworkBuffer *swap = buffer_new();
                buffer_move(generic_packet, sizeof(double), swap);
                buffer_swap_endianness(swap);
                double number;
                memmove(&number, swap->bytes, swap->size);
                buffer_free(swap);
                variable_pointer = &number;
                variable_size = sizeof(double);
                break;
            }
            case PKT_VARINT: {
                int32_t int32 = buffer_read_varint(generic_packet);
                MCVarInt *var_int = varint_encode(int32);
                variable_pointer = &var_int;
                variable_size = sizeof(MCVarInt *);
                break;
            }
            case PKT_UUID: {
                NetworkBuffer *uuid = buffer_new();
#ifdef __LITTLE_ENDIAN__
                buffer_swap_endianness(uuid);
#endif
                unsigned char *temp = malloc(2 * sizeof(uint64_t));
                buffer_poll(generic_packet, 2 * sizeof(uint64_t), temp);
                buffer_write(uuid, temp, 2 * sizeof(uint64_t));
                free(temp);
                variable_pointer = &uuid;
                variable_size = sizeof(NetworkBuffer *);
                break;
            }
            case PKT_BYTEARRAY: {
                NetworkBuffer *bytes = buffer_new();
                buffer_read_array(generic_packet, bytes);
                variable_pointer = &bytes;
                variable_size = sizeof(NetworkBuffer *);
                break;
            }
            case PKT_STRING_ARRAY: {
                NetworkBuffer *strings = buffer_new();
                uint32_t length = buffer_read_varint(generic_packet);
                for (int j = 0; j < length; ++j) {
                    buffer_read_array(generic_packet, strings);
                }
                variable_pointer = &strings;
                variable_size = sizeof(NetworkBuffer *);
                break;
            }
            case PKT_CHAT:
            case PKT_STRING: {
                NetworkBuffer *string = buffer_new();
                buffer_read_array(generic_packet, string);
                variable_pointer = &string;
                variable_size = sizeof(NetworkBuffer *);
                break;
            }
            case PKT_NBTTAG: {
                NetworkBuffer *nbt = buffer_new();
                consume_nbt_data(generic_packet);
                variable_pointer = &nbt;
                variable_size = sizeof(NetworkBuffer *);
                break;
            }
            case PKT_LOGIN_PROPERTIES: {
                NetworkBuffer *properties = buffer_new();
                uint16_t properties_size = generic_packet->size;
                unsigned char *temp = malloc(properties_size);
                buffer_poll(generic_packet, properties_size, temp);
                buffer_write(properties, temp, properties_size);
                free(temp);
                variable_pointer = &properties;
                variable_size = sizeof(NetworkBuffer *);
                break;
            }
            case PKT_SKIP: {
                sc_log(WARN, "Skipping packet fields for packet with id 0x%x.",
                       varint_decode((*header)->packet_id->bytes));
                return;
            }
            case PKT_IDENTIFIER:
            case PKT_VARLONG:
            case PKT_ENTITYMETA:
            case PKT_SLOT:
            case PKT_OPTIONAL:
            case PKT_ARRAY:
            case PKT_ENUM:
            default:
                sc_log(ERR, "Tried decoding unhandled generic_packet field %d", field);
                exit(EXIT_FAILURE);
        }
        memmove(ptr, variable_pointer, variable_size);
        ptr += variable_size;
    }
}
#pragma GCC pop_options


uint8_t receive_byte() {
    unsigned char byte;
    receive_wrapper(get_socket(), &byte, sizeof(byte));
    if (encryption_enabled) {
        int32_t outlen;
        unsigned char out;
        if (!EVP_DecryptUpdate(ctx_decrypt, &out, &outlen, &byte, sizeof(byte))) {
            sc_log(ERR, "OpenSSL decryption error");
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
        buffer_write(bytes, &byte, sizeof(byte));
    }
    return bytes;
}

int32_t receive_varint() {
    char current_byte;
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

#pragma GCC push_options
#pragma GCC optimize("O0")
GenericPacket *packet_receive_single() {
    GenericPacket *packet = malloc(sizeof(GenericPacket));
    if (!compression_enabled) {
        uint32_t length = receive_varint();
        uint32_t packet_id = receive_varint();

        packet->is_compressed = false;
        packet->uncompressed_length = length;
        packet->packet_id = packet_id;

        MCVarInt *temp = varint_encode(packet_id);
        packet->data = receive_bytes(length - temp->size);
        free(temp);
    } else {
        int32_t compressed_length = receive_varint();
        if (compressed_length == 0) {
            packet->uncompressed_length = 0;
            return packet;
        }
        int32_t uncompressed_length = receive_varint();
        MCVarInt *temp = varint_encode(uncompressed_length);
        NetworkBuffer *compressed_data = receive_bytes(compressed_length - temp->size);
        free(temp);

        if (uncompressed_length != 0) {
            char uncompressed_data_temp[uncompressed_length];
            uncompress((Bytef *) uncompressed_data_temp,
                       (uLongf *) &uncompressed_length,
                       (Bytef *) compressed_data->bytes,
                       (uLong) compressed_length);
            NetworkBuffer *uncompressed_data = buffer_new();
            buffer_write(
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
            buffer_free(compressed_data);
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
#pragma GCC pop_options

void *packet_receive(void *list_arg) {
    GenericPacketList *list = list_arg;
    while (true) {
        GenericPacket *packet = packet_receive_single();
        pthread_mutex_lock(list->mutex);
        if (list->first == NULL) {
            list->first = packet;
            list->last = packet;
        } else {
            list->last->next = packet;
            list->last = packet;
        }
        pthread_cond_broadcast(list->condition);
        pthread_mutex_unlock(list->mutex);
    }
    return NULL;
}

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

PacketHeader *handshake_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField fields[] = {
            PKT_VARINT,
            PKT_STRING,
            PKT_UINT16,
            PKT_VARINT
    };
    packet_generate_header(header, fields, 4, 0x00, SERVERBOUND, HANDSHAKE);
    return header;
}

PacketHeader *status_request_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    packet_generate_header(header, NULL, 0, 0x00, SERVERBOUND, STATUS);
    return header;
}

PacketHeader *status_response_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField fields[] = {PKT_STRING};
    packet_generate_header(header, fields, 1, 0x00, CLIENTBOUND, STATUS);
    return header;
}

PacketHeader *login_start_header_header() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_STRING,
            PKT_BOOL,
            PKT_UUID
    };
    packet_generate_header(header, typeArray, 3, 0x00, SERVERBOUND, LOGIN);
    return header;
}

PacketHeader *disconnect_login_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField fields[] = {
            PKT_STRING
    };
    packet_generate_header(header, fields, 1, 0x00, CLIENTBOUND, LOGIN);
    return header;
}

PacketHeader *encryption_response_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_BYTEARRAY,
            PKT_BYTEARRAY
    };
    packet_generate_header(header, typeArray, 2, 0x01, SERVERBOUND, LOGIN);
    return header;
}


PacketHeader *encryption_request_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_STRING,
            PKT_BYTEARRAY,
            PKT_BYTEARRAY
    };
    packet_generate_header(header, typeArray, 3, 0x01, CLIENTBOUND, LOGIN);
    return header;
}

PacketHeader *set_compression_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField types[] = {PKT_VARINT};
    packet_generate_header(header, types, 1, 0x03, CLIENTBOUND, LOGIN);
    return header;
}

// TODO: implement arrays
PacketHeader *login_success_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_UUID,
            PKT_STRING,
            PKT_VARINT,
            PKT_LOGIN_PROPERTIES
    };
    packet_generate_header(header, typeArray, 4, 0x02, CLIENTBOUND, LOGIN);
    return header;
}

PacketHeader *client_info_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
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
    packet_generate_header(header, typeArray, 8, 0x07, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *set_player_pos_and_rot_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_DOUBLE,
            PKT_DOUBLE,
            PKT_DOUBLE,
            PKT_FLOAT,
            PKT_FLOAT,
            PKT_BOOL
    };
    packet_generate_header(header, typeArray, 6, 0x14, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *client_command_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_VARINT
    };
    packet_generate_header(header, typeArray, 1, 0x06, SERVERBOUND, PLAY);
    return header;
}

PacketHeader *confirm_teleportation_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_VARINT
    };
    packet_generate_header(header, typeArray, 1, 0x00, SERVERBOUND, PLAY);
    return header;
}

PacketHeader *login_play_header_new(bool *hasDeathLocation) {
    PacketHeader *header = malloc(sizeof(PacketHeader));
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
    packet_generate_header(header, typeArray, 19, 0x25, CLIENTBOUND, PLAY);
    header->optionals[17] = hasDeathLocation;
    header->optionals[18] = hasDeathLocation;
    return header;
}


PacketHeader *disconnect_play_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_STRING
    };
    packet_generate_header(header, typeArray, 1, 0x17, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *synchronize_player_position_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
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
    packet_generate_header(header, typeArray, 8, 0x38, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *update_recipes_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_VARINT,
            PKT_STRING
    };
    packet_generate_header(header, typeArray, 2, 0x69, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *change_difficulty_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_UINT8,
            PKT_BOOL
    };
    packet_generate_header(header, typeArray, 2, 0x0b, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *player_abilities_cb_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_UINT8,
            PKT_FLOAT,
            PKT_FLOAT
    };
    packet_generate_header(header, typeArray, 3, 0x31, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *keep_alive_clientbound_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_UINT64
    };
    packet_generate_header(header, typeArray, 1, 0x1F, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *keep_alive_serverbound_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_UINT64
    };
    packet_generate_header(header, typeArray, 1, 0x11, SERVERBOUND, PLAY);
    return header;
}

//TODO: Packet has optional BitSet that is left out here
PacketHeader *player_chat_message_header_new(
        bool *has_signature,
        bool *has_unsigned_content,
        bool *has_target_network
) {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_UUID,
            PKT_VARINT,
            PKT_BOOL,
            PKT_BYTEARRAY,
            PKT_STRING,
            PKT_SKIP
            /**PKT_UINT64,
            PKT_UINT64,
            PKT_VARINT,
            PKT_PREV_MSG_ARRAY,
            PKT_BOOL,
            PKT_CHAT,
            PKT_VARINT,
            PKT_VARINT,
            PKT_CHAT,
            PKT_BOOL,
            PKT_CHAT **/
    };
    packet_generate_header(header, typeArray, 6, 0x31, CLIENTBOUND, PLAY);
    header->optionals[3] = has_signature;
    /**header->optionals[10] = has_unsigned_content;
    header->optionals[15] = has_target_network;**/
    return header;
}

//TODO: Complete packet
PacketHeader *chunk_data_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_INT32,
            PKT_INT32,
            PKT_NBTTAG,
            PKT_BYTEARRAY,
            PKT_SKIP
    };
    packet_generate_header(header, typeArray, 5, 0x20, CLIENTBOUND, PLAY);
    return header;
}

PacketHeader *unload_chunk_header_new() {
    PacketHeader *header = malloc(sizeof(PacketHeader));
    PacketField typeArray[] = {
            PKT_INT32,
            PKT_INT32
    };
    packet_generate_header(header, typeArray, 2, 0x1b, CLIENTBOUND, PLAY);
    return header;
}