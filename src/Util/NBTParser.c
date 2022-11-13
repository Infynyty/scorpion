//
// Created by Kasimir on 12.11.2022.
//

#include <stdint.h>
#include <stdbool.h>
#include "NBTParser.h"
#include "NetworkBuffer.h"
#include "Logger.h"

typedef enum NBT_TAGS {
    TAG_END, TAG_BYTE, TAG_SHORT, TAG_INT, TAG_LONG, TAG_FLOAT, TAG_DOUBLE, TAG_BYTE_ARRAY, TAG_STRING, TAG_LIST,
    TAG_COMPOUND, TAG_INT_ARRAY, TAG_LONG_ARRAY
} NBT_TAGS;

void handle_compound(SocketWrapper *socket);
void handle_string(SocketWrapper *socket);

void consume_nbt_data(SocketWrapper *socket) {
//    NetworkBuffer *test = buffer_new();
//    buffer_receive(test, socket, 200);

    NBT_TAGS element_tag = buffer_receive_uint8_t(socket);
    handle_string(socket);
    handle_compound(socket);
}

void handle_end(SocketWrapper *socket) {
    return;
}

void handle_byte(SocketWrapper *socket) {
    buffer_receive_uint8_t(socket);
}

void handle_short(SocketWrapper *socket) {
    buffer_receive_int16_t(socket);
}

void handle_int(SocketWrapper *socket) {
    buffer_receive_int32_t(socket);
}

void handle_long(SocketWrapper *socket) {
    buffer_receive_int64_t(socket);
}

void handle_float(SocketWrapper *socket) {
    buffer_receive_float(socket);
}

void handle_double(SocketWrapper *socket) {
    buffer_receive_double(socket);
}

void handle_byte_array(SocketWrapper *socket) {
    int32_t length = buffer_receive_int32_t(socket);
    for (int i = 0; i < length; ++i) {
        handle_byte(socket);
    }
}

void handle_string(SocketWrapper *socket) {
    uint16_t length = buffer_receive_uint16_t(socket);
    NetworkBuffer *string = buffer_new();
    buffer_receive(string, socket, length);
    buffer_free(string);
}

void handle_int_array(SocketWrapper *socket) {
    int32_t length = buffer_receive_int32_t(socket);
    for (int i = 0; i < length; ++i) {
        handle_int(socket);
    }
}

void handle_long_array(SocketWrapper *socket) {
    int32_t length = buffer_receive_int32_t(socket);
    for (int i = 0; i < length; ++i) {
        handle_long(socket);
    }
}



void handle_list(SocketWrapper *socket) {
    NBT_TAGS tag = buffer_receive_uint8_t(socket);
    int32_t length = buffer_receive_int32_t(socket);
    for (int i = 0; i < length; ++i) {
        switch (tag) {
            case TAG_END:
                handle_end(socket);
                return;
            case TAG_BYTE:
                handle_byte(socket);
                break;
            case TAG_SHORT:
                handle_short(socket);
                break;
            case TAG_INT:
                handle_int(socket);
                break;
            case TAG_LONG:
                handle_long(socket);
                break;
            case TAG_FLOAT:
                handle_float(socket);
                break;
            case TAG_DOUBLE:
                handle_double(socket);
                break;
            case TAG_BYTE_ARRAY:
                handle_byte_array(socket);
                break;
            case TAG_STRING:
                handle_string(socket);
                break;
            case TAG_LIST:
                handle_list(socket);
                break;
            case TAG_COMPOUND:
                handle_compound(socket);
                break;
            case TAG_INT_ARRAY:
                handle_int_array(socket);
                break;
            case TAG_LONG_ARRAY:
                handle_long_array(socket);
                break;
        }
    }
}

void handle_compound(SocketWrapper *socket) {
    while (1) {
        NBT_TAGS element_tag = buffer_receive_uint8_t(socket);
        if (element_tag == TAG_END) return;
        handle_string(socket);
        switch (element_tag) {
            case TAG_END:
                handle_end(socket);
                return;
            case TAG_BYTE:
                handle_byte(socket);
                break;
            case TAG_SHORT:
                handle_short(socket);
                break;
            case TAG_INT:
                handle_int(socket);
                break;
            case TAG_LONG:
                handle_long(socket);
                break;
            case TAG_FLOAT:
                handle_float(socket);
                break;
            case TAG_DOUBLE:
                handle_double(socket);
                break;
            case TAG_BYTE_ARRAY:
                handle_byte_array(socket);
                break;
            case TAG_STRING:
                handle_string(socket);
                break;
            case TAG_LIST:
                handle_list(socket);
                break;
            case TAG_COMPOUND:
                handle_compound(socket);
                break;
            case TAG_INT_ARRAY:
                handle_int_array(socket);
                break;
            case TAG_LONG_ARRAY:
                handle_long_array(socket);
                break;
        }
    }
}