//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_PLAYSTATE_H
#define CMC_PLAYSTATE_H

#include <stdlib.h>
#include "PlayState.h"

typedef struct PlayState {
    ServerState *serverState;
    ClientState *clientState;
    WorldState *worldState;
} PlayState;

PlayState *play_state_new();

void play_state_free(PlayState *state);

#endif //CMC_PLAYSTATE_H
