//
// Created by Kasimir Stadie on 21.10.22.
//

#include "WorldState.h"
#include "Packets.h"
#include "Logger.h"
#include <stdint.h>
#include <stdlib.h>
#include <json-c/json.h>

#define SECTIONS_IN_CHUNK_COLUMN 24




void init_global_palette(WorldState *world_state) {
    json_object *report = json_object_from_file("blocks.json");;
    if (report == NULL) {
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
        buffer_free(data->block_states[i]);
        buffer_free(data->biomes[i]);
    }
}

WorldState *world_state_new() {
    return malloc(sizeof(WorldState));
}

void world_state_free(WorldState *state) {
    free(state);
}

ChunkData *chunk_data_new(ChunkDataPacket *packet) {
    ChunkData *data = malloc(sizeof(ChunkData));
    data->x = packet->chunk_x;
    data->z = packet->chunk_z;
    NetworkBuffer **block_states = malloc(sizeof(NetworkBuffer) * SECTIONS_IN_CHUNK_COLUMN);
    NetworkBuffer *raw_data = buffer_clone(packet->data);
    for (int i = 0; i < SECTIONS_IN_CHUNK_COLUMN; i++) {
        buffer_read(uint8_t, raw_data);
        //inside palette container
        NetworkBuffer *chunk_sec_data = buffer_new();
        uint8_t bits_per_entry = buffer_read(uint8_t, raw_data);
        buffer_write(chunk_sec_data, &bits_per_entry, sizeof(uint8_t));
        if (bits_per_entry == 0) {
            int32_t single_id = buffer_read_varint(raw_data);
            MCVarInt *single_id_reencoded = varint_encode(single_id);
            buffer_write(chunk_sec_data, single_id_reencoded->bytes, single_id_reencoded->size);
        } else if (bits_per_entry < 8) {
            int32_t no_of_ids = buffer_read_varint(raw_data);
            MCVarInt *no_of_ids_reencoded = varint_encode(no_of_ids);
            buffer_write(chunk_sec_data, no_of_ids_reencoded->bytes, no_of_ids_reencoded->size);
            for (int j = 0; j < no_of_ids; j++) {
                int32_t id = buffer_read_varint(raw_data);
                MCVarInt *id_reencoded = varint_encode(id);
                buffer_write(chunk_sec_data, id_reencoded->bytes, id_reencoded->size);
            }
        }
        int32_t data_array_len = buffer_read_varint(raw_data);
        MCVarInt *data_array_len_reencoded = varint_encode(data_array_len);
        buffer_write(chunk_sec_data, data_array_len_reencoded->bytes, data_array_len_reencoded->size);

        buffer_move(raw_data, data_array_len * sizeof(uint64_t), chunk_sec_data);

        block_states[i] = chunk_sec_data;
    }
    buffer_free(raw_data);

    return data;
}

void add_chunk(ChunkDataPacket *chunk, WorldState *state) {
    ChunkData *new_chunk = chunk_data_new(chunk);
    ChunkData *current_chunk = state->chunks;
    ChunkData *prev_chunk;
    if (current_chunk == NULL) {
        current_chunk->next = new_chunk;
        return;
    }
    if (current_chunk->x == chunk->chunk_x && current_chunk->z == chunk->chunk_x) {
        state->chunks = new_chunk;
        new_chunk->next = current_chunk->next;
        chunk_data_free(current_chunk);
    }
    prev_chunk = current_chunk;
    current_chunk = current_chunk->next;
    while (current_chunk != NULL) {
        if (current_chunk->x == chunk->chunk_x && current_chunk->z == chunk->chunk_x) {
            prev_chunk->next = new_chunk;
            new_chunk->next = current_chunk->next;
            chunk_data_free(current_chunk);
            return;
        }
        prev_chunk = current_chunk;
        current_chunk = current_chunk->next;
    }
    prev_chunk->next = current_chunk;
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



ChunkData *handle_chunk_data(NetworkBuffer *chunk_data) {
    uint8_t bits_per_entry = buffer_read(uint8_t, chunk_data);
    PalettedContainerType type;
    if (bits_per_entry == 0) {
        type = SINGLE_VALUE;
    } else if (bits_per_entry <= 4) {
        type = INDIRECT;
    } else {
        type = DIRECT;
    }

}



