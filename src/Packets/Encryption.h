#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include "Packets.h"

/**
 * Fills in a given EncryptionResponsePacket.
 * @param packet        Overwrites the contents of the packet, so that they contain the encrypted shared secret and verify token.
 * @param public_key    The servers public key in DER encoding.
 * @param verify_token  The verify token that the server sent with the encryption request.
 * @param secret        Overwrites the contents of secret, so that they contain the unencrypted, generated secret.
 */
void encryption_response_generate(
        EncryptionResponsePacket *packet,
        NetworkBuffer *public_key,
        NetworkBuffer *verify_token,
        NetworkBuffer *secret
);

#endif //ENCRYPTION_H