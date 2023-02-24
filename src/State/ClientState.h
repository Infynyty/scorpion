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

typedef struct ClientStatestruct {
    ProfileInformation *profile_info;
	Position *position;
	bool is_hardcore;
	uint8_t gamemode;
	uint8_t previous_gamemode;
	int teleportID;
	Position *last_death_position;
} ClientState;

ClientState *client_state_new();

void client_state_free(ClientState *clientState);

#endif //CMC_CLIENTSTATE_H
