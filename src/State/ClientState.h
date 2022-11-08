//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_CLIENTSTATE_H
#define CMC_CLIENTSTATE_H

#include <stdint.h>
#include <stdbool.h>
#include "Position.h"

#define SURVIVAL    0
#define CREATIVE    1
#define ADVENTURE   2
#define SPECTATOR   3

typedef struct ClientStatestruct {
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
