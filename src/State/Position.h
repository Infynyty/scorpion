//
// Created by Kasimir Stadie on 21.10.22.
//

#ifndef CMC_POSITION_H
#define CMC_POSITION_H

typedef struct Dimension {
    int name_length;
    char *name;
    int dimension_type_length;
    char *dimension_type;
} Dimension;

typedef struct Position {
    double x;
    double y;
    double z;
    float yaw;
    float pitch;
    Dimension *dimension;
} Position;

Position *position_new(double x, double y, double z, float yaw, float pitch);

#endif //CMC_POSITION_H
