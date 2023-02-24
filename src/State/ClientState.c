//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdlib.h>
#include "ClientState.h"


ClientState *client_state_new() {
	ClientState *state = malloc(sizeof(ClientState));
    ProfileInformation *info = malloc(sizeof(ProfileInformation));
    state->profile_info = info;
    return state;
}



void client_state_free(ClientState *clientState) {
    free(clientState->profile_info);
    free(clientState->position);
    free(clientState->last_death_position);
	free(clientState);
}