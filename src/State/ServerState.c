//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdbool.h>
#include "ServerState.h"

struct ServerState {
	int max_players;
	int view_distance;
	int simulation_distance;
	bool reduced_debug_info;
	bool enable_respawn_screen;
	bool is_debug;
	bool is_flat;
};