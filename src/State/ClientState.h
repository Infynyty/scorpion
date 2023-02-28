//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_CLIENTSTATE_H
#define CMC_CLIENTSTATE_H

#include <stdint.h>
#include <stdbool.h>
#include "Position.h"
#include "NetworkBuffer.h"

typedef struct ProfileInformation {
    NetworkBuffer *name;
    NetworkBuffer *uuid;
} ProfileInformation;


typedef struct AuthenticationDetails {
    NetworkBuffer *ms_access_token;
    NetworkBuffer *xbl_token;
    NetworkBuffer *xsts_token;
    NetworkBuffer *mc_token;
    NetworkBuffer *player_hash;
} AuthenticationDetails;

typedef struct ClientStatestruct {
    ProfileInformation *profile_info;
    AuthenticationDetails *auth_details;
    bool is_spawned;
	Position *position;
	bool is_hardcore;
	uint8_t gamemode;
	uint8_t previous_gamemode;
	int teleportID;
	Position *last_death_position;
} ClientState;

AuthenticationDetails *authentication_details_new();

void authentication_details_free(AuthenticationDetails *details);

ClientState *client_state_new();

void client_state_free(ClientState *clientState);

#endif //CMC_CLIENTSTATE_H
