#ifndef SCORPION_AUTHENTICATION_H
#define SCORPION_AUTHENTICATION_H

#include "NetworkBuffer.h"
#include "ClientState.h"
#include "Packets.h"

/**
 * Authenticates a client using the Microsoft accounts API. The user will be asked to log into Microsoft in their browser
 * using a link that will be written to the console. If the user is already logged in, their token will be stored in a file
 * called ".token".
 *
 * @param state Used to save the received credentials.
 */
void authenticate(ClientState *state);

/**
 * Authenticates a user to join the server that the application is connected to. The user must have previously logged into
 * their account with <authenticate> and have received an EncryptionRequestPacket.
 * @param packet                Is required to send the correct hash to the API.
 * @param unencrypted_secret    Is required to send the correct hash to the API.
 * @param client                Is required to access the correct access token for the Minecraft API.
 */
void authenticate_server(EncryptionRequestPacket *packet, NetworkBuffer *unencrypted_secret, ClientState *client);

#endif //SCORPION_AUTHENTICATION_H
