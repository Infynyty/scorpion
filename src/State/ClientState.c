//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdlib.h>
#include "ClientState.h"

AuthenticationDetails *authentication_details_new() {
    AuthenticationDetails *details = malloc(sizeof(AuthenticationDetails));
    details->ms_access_token = buffer_new();
    details->xbl_token = buffer_new();
    details->xsts_token = buffer_new();
    details->mc_token = buffer_new();
    details->player_hash = buffer_new();
    return details;
}

void authentication_details_free(AuthenticationDetails *details) {
    buffer_free(details->ms_access_token);
    buffer_free(details->xbl_token);
    buffer_free(details->xsts_token);
    buffer_free(details->mc_token);
    buffer_free(details->player_hash);
    free(details);
}

void profile_info_free(ProfileInformation *info) {
    buffer_free(info->name);
    buffer_free(info->uuid);
    free(info);
}


ClientState *client_state_new() {
	ClientState *state = malloc(sizeof(ClientState));
    ProfileInformation *info = malloc(sizeof(ProfileInformation));
    state->profile_info = info;
    state->auth_details = authentication_details_new();
    return state;
}

void client_state_free(ClientState *clientState) {
    authentication_details_free(clientState->auth_details);
    profile_info_free(clientState->profile_info);
    free(clientState->profile_info);
    free(clientState->position);
    free(clientState->last_death_position);
	free(clientState);
}