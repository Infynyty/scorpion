#ifndef SCORPION_ASTAR_H
#define SCORPION_ASTAR_H

#include <stddef.h>
#include "Position.h"
#include <stdint.h>

/**
 * Finds a path from position start to position goal using A*.
 * @param start
 * @param goal
 * @param state
 * @param counter   The number of positions in the final array will be copied to this address.
 * @return  An array of positions to the goal or NULL if no path was found.
 */
Position** find_path(Position *start, Position *goal, WorldState *state, uint32_t *counter);

#endif //SCORPION_ASTAR_H
