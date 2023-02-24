//
// Created by Kasimir Stadie on 11.02.23.
//

#ifndef SCORPION_AUTHENTICATION_H
#define SCORPION_AUTHENTICATION_H

#include "NetworkBuffer.h"
#include "ClientState.h"

typedef struct AuthenticationDetails AuthenticationDetails;

AuthenticationDetails *authenticate(ClientState *state);

#endif //SCORPION_AUTHENTICATION_H
