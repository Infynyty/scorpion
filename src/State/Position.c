//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdlib.h>
#include "Position.h"

typedef struct Position {
	double x;
	double y;
	double z;
	float yaw;
	float pitch;
} Position;

Position *position_new(double x, double y, double z, float yaw, float pitch) {
	Position *position = malloc(sizeof(Position));
	position->x = x;
	position->y = y;
	position->z = z;
	position->yaw = yaw;
	position->pitch = pitch;
	return position;
}

void position_change_position_relative(Position *position, double x_delta, double y_delta, double z_delta) {
	position->x += x_delta;
	position->y += y_delta;
	position->z += z_delta;
}

void position_change_rotation_relative(Position *position, float yaw_delta, float pitch_delta) {
	position->yaw += yaw_delta;
	position->pitch += pitch_delta;
}