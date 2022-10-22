//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdlib.h>
#include "ClientState.h"
#include "Position.h"
#include "MCVarInt.h"


struct ClientState {
	Position *position;
	int teleportID;
};

ClientState *client_state_new() {
	return (ClientState *) malloc(sizeof(ClientState));
}

void client_state_free(ClientState *clientState) {
	free(clientState->position);
	free(clientState);
}

void client_update_position(ClientState *client, Position *position) {
	client->position = position;
}

void client_set_teleportID(ClientState *client, int teleportID) {
	client->teleportID = teleportID;
}

int client_get_teleportID(ClientState *client) {
	return client->teleportID;
}