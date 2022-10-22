//
// Created by Kasimir Stadie on 21.10.22.
//

#include "SocketWrapper.h"
#include "../../../State/PlayState.h"
#include "../../../State/ClientState.h"

#ifndef CMC_SYNCHRONIZEPLAYERPOSITION_H
#define CMC_SYNCHRONIZEPLAYERPOSITION_H

void synchronize_player_position_packet_handle(SocketWrapper *socketWrapper, ClientState *clientState);

#endif //CMC_SYNCHRONIZEPLAYERPOSITION_H
