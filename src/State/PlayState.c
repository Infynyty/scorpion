//
// Created by Kasimir Stadie on 21.10.22.
//

#include "ServerState.h"
#include "ClientState.h"
#include "WorldState.h"
#include "PlayState.h"


PlayState *play_state_new() {
    PlayState *state = malloc(sizeof(PlayState));
    state->clientState = client_state_new();
    state->worldState = world_state_new();
    state->serverState = serverstate_new();
    return state;
}

void play_state_free(PlayState *state) {
    client_state_free(state->clientState);
    world_state_free(state->worldState);
    serverstate_free(state->serverState);
    free(state);
}