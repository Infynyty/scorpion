#include <unistd.h>
#include "Movement.h"
#include <math.h>
#include "PlayState.h"

#define PACKETS_PER_BLOCK 10
#define DISTANCE_PER_PACKET 0.1f

void move_player(Position *goal, PlayState *state) {
    Position *start = state->clientState->position;
    double x_diff = goal->x - start->x;
    double z_diff = goal->z - start->z;

    double x_packets_to_send = fabs(x_diff) * PACKETS_PER_BLOCK;
    double z_packets_to_send = fabs(z_diff) * PACKETS_PER_BLOCK;

    SetPlayerPosAndRotPacket pos = {._header = set_player_pos_and_rot_header_new()};
    pos.x = start->x;
    pos.y = start->y;
    pos.z = start->z;
    for (int i = 0; i < x_packets_to_send; ++i) {
        pos.x += x_diff > 0 ? DISTANCE_PER_PACKET : -DISTANCE_PER_PACKET;
        packet_send(&pos._header);
        position_change_position_relative(state->clientState->position, x_diff > 0 ? DISTANCE_PER_PACKET : -DISTANCE_PER_PACKET, 0, 0);
        usleep(1000 * 20);
    }
    for (int i = 0; i < z_packets_to_send; ++i) {
        pos.z += z_diff > 0 ? DISTANCE_PER_PACKET : -DISTANCE_PER_PACKET;
        packet_send(&pos._header);
        usleep(1000 * 20);
        position_change_position_relative(state->clientState->position, 0, 0, z_diff > 0 ? DISTANCE_PER_PACKET : -DISTANCE_PER_PACKET);
    }
    packet_free(&pos._header);
}