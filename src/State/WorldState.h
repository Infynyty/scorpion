//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_WORLDSTATE_H
#define CMC_WORLDSTATE_H

#include "WorldState.h"
#include "NetworkBuffer.h"
#include "Position.h"
#include "Packets.h"

#define BLOCK_STATES 23232

typedef enum PaletteType {
    SINGLE_VALUE, INDIRECT, DIRECT
} PaletteType;

typedef struct PalettedContainer {
    uint8_t bits_per_entry;
    PaletteType palette_type;
    int32_t *palette;
    NetworkBuffer *data;
} PalettedContainer;

typedef struct ChunkData {
    int32_t x;
    int32_t z;
    PalettedContainer **block_states;
    PalettedContainer **biomes;
    struct ChunkData *next;
} ChunkData;

typedef struct BlockState {
    int32_t id;
    char *name;
} BlockState;

typedef struct WorldState {
    int dimension_count;
    Dimension *dimension_array;
    long hashed_seed;
    uint16_t no_of_loaded_chunks;
    ChunkData *chunks;
    BlockState* global_palette[BLOCK_STATES];
} WorldState;

WorldState *world_state_new();

void world_state_free(WorldState *state);

ChunkData *handle_chunk_data(NetworkBuffer *chunk_data);

void init_global_palette(WorldState *world_state);

void add_chunk(ChunkDataPacket *chunk, WorldState *state);

void remove_chunk(UnloadChunkPacket *packet, WorldState *state);

BlockState *get_block_at(Position *position, WorldState *state);


#endif //CMC_WORLDSTATE_H
