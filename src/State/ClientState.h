//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_CLIENTSTATE_H
#define CMC_CLIENTSTATE_H

#include "Position.h"

typedef struct ClientState ClientState;

ClientState *client_state_new();

void client_state_free(ClientState *clientState);

void client_update_position(ClientState *client, Position *position);

void client_set_teleportID(ClientState *client, int teleportID);

int client_get_teleportID(ClientState *client);

#endif //CMC_CLIENTSTATE_H
