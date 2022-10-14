//
// Created by Kasimir on 14.10.2022.
//

#ifndef CMC_PACKETHANDLER_H
#define CMC_PACKETHANDLER_H

#include <winsock2.h>
#include "../../Util/ConnectionState/ConnectionState.h"

void handle_incoming_packet(SOCKET socket, enum ConnectionState connectionState);

#endif //CMC_PACKETHANDLER_H
