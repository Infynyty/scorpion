//
// Created by Kasimir Stadie on 21.10.22.
//

#include "PlayState.h"
#include "ServerState.h"
#include "ClientState.h"
#include "WorldState.h"


struct PlayState {
	ServerState *serverState;
	ClientState *clientState;
	WorldState *worldState;
};