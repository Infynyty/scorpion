//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdbool.h>
#include "ServerState.h"
#include "NetworkBuffer.h"

ServerState *serverstate_new() {
    return malloc(sizeof(ServerState));
}

void serverstate_free(ServerState *state) {
    free(state->public_key);
    free(state->verify_token);
    free(state);
}