//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_WORLDSTATE_H
#define CMC_WORLDSTATE_H

#include "WorldState.h"
#include "NetworkBuffer.h"

WorldState *world_state_new();

void world_state_free(WorldState *state);

ChunkData *handle_chunk_data(NetworkBuffer *chunk_data);


#endif //CMC_WORLDSTATE_H
