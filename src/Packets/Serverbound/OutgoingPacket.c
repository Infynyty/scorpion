//
// Created by Kasimir on 04.11.2022.
//

#include <unistd.h>
#include <stdbool.h>
#include "OutgoingPacket.h"
#include "MCVarInt.h"
#include "NetworkBuffer.h"


uint8_t get_types_size(Types type) {
    switch (type) {
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
            return 1;
    }
}

void send_pkt_wrapper(PacketWrapper *packet) {
    NetworkBuffer *buffer = buffer_new();

    for (int i = 0; i < packet->members; ++i) {
        void *current_byte = packet++;
        Types m_type = packet->member_types[i];
        switch (m_type) {
            case PKT_VARINT: {
                MCVarInt *varInt = (MCVarInt *) &current_byte;
                buffer_write_little_endian(buffer, varInt->bytes, varInt->length);
                break;
            }
        }
        current_byte += get_types_size(m_type);
    }
}

void packet_free(PacketWrapper *packet) {
    void **ptr = (void **) packet++;
    for (int i = 0; i < packet->members; ++i) {
        Types m_type = packet->member_types[i];
        switch (m_type) {
            case PKT_VARINT:
            case PKT_VARLONG:
            case PKT_CHAT:
                free(&ptr); //TODO: Check!
            default:
                ptr += get_types_size(m_type);
        }
    }
}