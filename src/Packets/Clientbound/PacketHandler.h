//
// Created by Kasimir on 14.10.2022.
//

#ifndef CMC_PACKETHANDLER_H
#define CMC_PACKETHANDLER_H


#include "../../Util/ConnectionState/ConnectionState.h"
#include "../../Util/SocketWrapper.h"

void handle_incoming_packet(SocketWrapper *socket);

#endif //CMC_PACKETHANDLER_H
