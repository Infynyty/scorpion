//
// Created by Kasimir on 12.11.2022.
//

#ifndef CMC_NBTPARSER_H
#define CMC_NBTPARSER_H

#include "SocketWrapper.h"
#include "NetworkBuffer.h"

void consume_nbt_data(NetworkBuffer *buffer);

#endif //CMC_NBTPARSER_H
