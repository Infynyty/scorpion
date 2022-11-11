//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdlib.h>
#include <stdbool.h>
#include "ClientState.h"
#include "Position.h"
#include "MCVarInt.h"
#include "WorldState.h"


struct ClientState;


ClientState *client_state_new() {
	return (ClientState *) malloc(sizeof(ClientState));
}

void client_state_free(ClientState *clientState) {
//	free(clientState->position);
	free(clientState);
}