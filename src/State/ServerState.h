#ifndef CMC_SERVERSTATE_H
#define CMC_SERVERSTATE_H

#include "NetworkBuffer.h"
#include <stdbool.h>

typedef struct ServerState {
    int max_players;
    int view_distance;
    int simulation_distance;
    bool reduced_debug_info;
    bool enable_respawn_screen;
    bool is_debug;
    bool is_flat;
    NetworkBuffer *public_key;
    NetworkBuffer *verify_token;
} ServerState;

ServerState *serverstate_new();

void serverstate_free(ServerState *state);

#endif //CMC_SERVERSTATE_H
