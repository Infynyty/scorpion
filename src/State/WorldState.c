//
// Created by Kasimir Stadie on 21.10.22.
//

#include "WorldState.h"
#include "Packets.h"
#include "Logger.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <json-c/json.h>
#include <math.h>

#define SECTIONS_IN_CHUNK_COLUMN 24




void init_global_palette(WorldState *world_state) {
    json_object *report = json_object_from_file("blocks.json");
    if (report == NULL) {
        cmc_log(INFO, "Error: %s", json_util_get_last_err());
        cmc_log(ERR, "Could not find blocks report. Exiting...");
        exit(EXIT_FAILURE);
    }

    struct json_object_iterator iterator = json_object_iter_begin(report);
    struct json_object_iterator iterator_end = json_object_iter_end(report);

    while(!json_object_iter_equal(&iterator, &iterator_end)) {
        json_object *block = json_object_iter_peek_value(&iterator);
        const char *block_name_json = json_object_iter_peek_name(&iterator);
        char *block_name = malloc(strlen(block_name_json) + 1);
        strcpy(block_name, block_name_json);
        json_object *states = json_object_object_get(block, "states");
        for (int i = 0; i < json_object_array_length(states); i++) {
            json_object *state = json_object_array_get_idx(states, i);
            int32_t id = json_object_get_int(json_object_object_get(state, "id"));

            BlockState *block_state = malloc(sizeof(BlockState));
            block_state->id = id;
            block_state->name = block_name;
            world_state->global_palette[id] = block_state;
        }
        json_object_iter_next(&iterator);
    }
    json_object_put(report);
}

void chunk_data_free(ChunkData *data) {
    for (int i = 0; i < SECTIONS_IN_CHUNK_COLUMN; i++) {
        free(data->block_states[i]->palette);
        buffer_free(data->block_states[i]->data);
        free(data->biomes[i]->palette);
        buffer_free(data->biomes[i]->data);
    }
}

WorldState *world_state_new() {
    return malloc(sizeof(WorldState));
}

void world_state_free(WorldState *state) {
    free(state);
}

PalettedContainer *block_palettet_container_new(NetworkBuffer *raw_data) {
    PalettedContainer *container = malloc(sizeof(PalettedContainer));
    container->data = NULL;
    container->palette = NULL;
    uint8_t bits_per_entry = buffer_read(uint8_t, raw_data);

    if (bits_per_entry == 0) {
        container->bits_per_entry = 0;
        container->palette_type = SINGLE_VALUE;
    } else if (bits_per_entry <= 4) {
        container->bits_per_entry = 4;
        container->palette_type = INDIRECT;
    } else if (bits_per_entry <= 8){
        container->bits_per_entry = bits_per_entry;
        container->palette_type = INDIRECT;
    } else {
        container->bits_per_entry = ceil(log2(BLOCK_STATES));
        container->palette_type = DIRECT;
    }

    if (container->palette_type == SINGLE_VALUE) {
        container->palette = malloc(1 * sizeof(int32_t));
        container->palette[0] = buffer_read_varint(raw_data);
        int32_t data_length = buffer_read_varint(raw_data);
        return container;
    } else if (container->palette_type == INDIRECT) {
        int32_t length = buffer_read_varint(raw_data);
        container->palette = malloc(length * sizeof(int32_t));
        for (int i = 0; i < length; i++) {
            container->palette[i] = buffer_read_varint(raw_data);
        }
    }
    int32_t data_length = buffer_read_varint(raw_data);
    container->data = buffer_new();
    buffer_move(raw_data, data_length * sizeof(uint64_t), container->data);
    return container;
}

PalettedContainer *biomes_palettet_container_new(NetworkBuffer *raw_data) {
    PalettedContainer *container = malloc(sizeof(PalettedContainer));
    container->data = NULL;
    container->palette = NULL;
    uint8_t bits_per_entry = buffer_read(uint8_t, raw_data);

    if (bits_per_entry == 0) {
        container->bits_per_entry = 0;
        container->palette_type = SINGLE_VALUE;
    } else if (bits_per_entry <= 3){
        container->bits_per_entry = bits_per_entry;
        container->palette_type = INDIRECT;
    } else {
        container->bits_per_entry = 6;
        container->palette_type = DIRECT;
    }

    if (container->palette_type == SINGLE_VALUE) {
        container->palette = malloc(1 * sizeof(int32_t));
        container->palette[0] = buffer_read_varint(raw_data);
        int32_t data_length = buffer_read_varint(raw_data);
        return container;
    } else if (container->palette_type == INDIRECT) {
        int32_t length = buffer_read_varint(raw_data);
        container->palette = malloc(length * sizeof(int32_t));
        for (int i = 0; i < length; i++) {
            container->palette[i] = buffer_read_varint(raw_data);
        }
    }
    int32_t data_length = buffer_read_varint(raw_data);
    container->data = buffer_new();
    buffer_move(raw_data, data_length * sizeof(uint64_t), container->data);
    return container;
}

ChunkData *chunk_data_new(ChunkDataPacket *packet) {
    ChunkData *data = malloc(sizeof(ChunkData));
    data->x = packet->chunk_x;
    data->z = packet->chunk_z;
    PalettedContainer **block_states = malloc(sizeof(PalettedContainer) * SECTIONS_IN_CHUNK_COLUMN);
    PalettedContainer **biome_states = malloc(sizeof(PalettedContainer) * SECTIONS_IN_CHUNK_COLUMN);
    NetworkBuffer *raw_data = buffer_clone(packet->data);
    for (int i = 0; i < SECTIONS_IN_CHUNK_COLUMN; i++) {
        uint16_t non_air_blocks = be16toh(buffer_read(uint16_t, raw_data));
        PalettedContainer *container = block_palettet_container_new(raw_data);
        block_states[i] = container;
        PalettedContainer *biomes = biomes_palettet_container_new(raw_data);
        biome_states[i] = biomes;
    }
    data->block_states = block_states;
    data->biomes = biome_states;
    data->next = NULL;
    buffer_free(raw_data);
    return data;
}

ChunkData *get_chunk(int32_t x, int32_t z, WorldState *state) {
    ChunkData *current_chunk = state->chunks;
    while(current_chunk != NULL) {
        if (current_chunk->x == x && current_chunk->z == z) {
            return current_chunk;
        }
        current_chunk = current_chunk->next;
    }
    return NULL;
}

void add_chunk(ChunkDataPacket *chunk, WorldState *state) {
    ChunkData *new_chunk = chunk_data_new(chunk);
    ChunkData *current_chunk = state->chunks;
    ChunkData *prev_chunk;
    if (current_chunk == NULL) {
        state->chunks = new_chunk;
        return;
    }
    if (current_chunk->x == chunk->chunk_x && current_chunk->z == chunk->chunk_z) {
        state->chunks = new_chunk;
        new_chunk->next = current_chunk->next;
        chunk_data_free(current_chunk);
    }
    prev_chunk = current_chunk;
    current_chunk = current_chunk->next;
    while (current_chunk != NULL) {
        if (current_chunk->x == chunk->chunk_x && current_chunk->z == chunk->chunk_z) {
            prev_chunk->next = new_chunk;
            new_chunk->next = current_chunk->next;
            chunk_data_free(current_chunk);
            return;
        }
        prev_chunk = current_chunk;
        current_chunk = current_chunk->next;
    }
    prev_chunk->next = new_chunk;
}

void remove_chunk(UnloadChunkPacket *packet, WorldState *state) {
    ChunkData *current_chunk = state->chunks;
    ChunkData *prev_chunk;
    if (current_chunk == NULL) return;
    if (current_chunk->x == packet->chunk_x && current_chunk->z == packet->chunk_z) {
        state->chunks = current_chunk->next;
        chunk_data_free(current_chunk);
    }
    prev_chunk = current_chunk;
    current_chunk = current_chunk->next;
    while (current_chunk != NULL) {
        if (current_chunk->x == packet->chunk_x && current_chunk->z == packet->chunk_z) {
            prev_chunk->next = current_chunk->next;
            chunk_data_free(current_chunk);
        }
        prev_chunk = current_chunk;
        current_chunk = current_chunk->next;
    }
}

BlockState *get_block_at(Position *position, WorldState *state) {
    int32_t chunk_x = (int32_t) floor(position->x / 16.0);
    int32_t chunk_z = (int32_t) floor(position->z / 16.0);
    int8_t chunk_section = (int8_t) ((position->y / 16) + 4);

    ChunkData *chunk = get_chunk(chunk_x, chunk_z, state);
    if (chunk == NULL) return NULL;
    PalettedContainer *section = chunk->block_states[chunk_section];

    if (section->palette_type == SINGLE_VALUE) {
        return state->global_palette[section->palette[0]];
    }

    int8_t y_layer = ((int) position->y % 16);
    y_layer = y_layer < 0 ? y_layer + 16 : y_layer;
    int8_t z_row = ((int) position->z % 16);
    z_row = z_row < 0 ? z_row + 16 : z_row;
    int8_t x_pos = ((int) position->x % 16);
    x_pos = x_pos < 0 ? x_pos + 16 : x_pos;

    uint32_t block_index = 256 * y_layer + 16 * z_row + x_pos;
    uint32_t blocks_per_long = (sizeof(uint64_t) * 8) / section->bits_per_entry;
    uint32_t long_index = block_index / blocks_per_long;
    uint32_t block_index_in_long = block_index - (long_index * blocks_per_long);
    uint32_t block_bit_position = long_index * sizeof(uint64_t) * 8 + block_index_in_long * section->bits_per_entry;
    uint64_t block = *((uint64_t *) (section->data->bytes + (long_index * sizeof(uint64_t))));
    block = be64toh(block);

    int8_t offset_from_LSB = (int8_t) ((sizeof(block) * 8) - (section->bits_per_entry + block_bit_position % 64));
    int8_t offset_from_MSB = (int8_t) ((sizeof(block) * 8) - section->bits_per_entry);

    block <<= offset_from_LSB;
    block >>= offset_from_MSB;

    return state->global_palette[section->palette[block]];
}