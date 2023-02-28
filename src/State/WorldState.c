//
// Created by Kasimir Stadie on 21.10.22.
//

#include "WorldState.h"
#include "Position.h"
#include <stdint.h>
#include <stdlib.h>

#define BLOCK_STATES 23232

#define SECTIONS_IN_CHUNK_COLUMN 16

static uint8_t global_palette[BLOCK_STATES];

typedef enum PalettedContainerType {
    SINGLE_VALUE, INDIRECT, DIRECT
} PalettedContainerType;

typedef struct ChunkData {
    int32_t x;
    int32_t z;
    NetworkBuffer **block_states;
    NetworkBuffer **biomes;
} ChunkData;

typedef struct WorldState {
	int dimension_count;
	Dimension *dimension_array;
	long hashed_seed;
} WorldState;

WorldState *world_state_new() {
    return malloc(sizeof(WorldState));
}

void world_state_free(WorldState *state) {
    free(state);
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



