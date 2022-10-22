//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_POSITION_H
#define CMC_POSITION_H

typedef struct Position Position;

Position* position_new(double x, double y, double z, float yaw, float pitch);

#endif //CMC_POSITION_H
