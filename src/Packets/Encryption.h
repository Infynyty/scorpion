#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include "Packets.h"

void encryption_response_generate(
        EncryptionResponsePacket *packet,
        NetworkBuffer *public_key,
        NetworkBuffer *verify_token,
        NetworkBuffer *secret
);

#endif //ENCRYPTION_H