//
// Created by Kasimir Stadie on 11.02.23.
//

#ifndef SCORPION_AUTHENTICATION_H
#define SCORPION_AUTHENTICATION_H

#include "NetworkBuffer.h"
#include "ClientState.h"
#include "Packets.h"

void authenticate(ClientState *state);

void authenticate_server(EncryptionRequestPacket *packet, NetworkBuffer *unencrypted_secret, ClientState *client);

#endif //SCORPION_AUTHENTICATION_H
