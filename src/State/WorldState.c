//
// Created by Kasimir Stadie on 21.10.22.
//

#include "WorldState.h"
#include "Position.h"


struct WorldState {
	int dimension_count;
	Dimension *dimension_array;
	long hashed_seed;
};

