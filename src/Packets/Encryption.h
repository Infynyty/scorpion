#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include "Packets.h"

EncryptionResponsePacket *encryption_response_generate(NetworkBuffer *public_key, NetworkBuffer *verify_token, NetworkBuffer *secret);

#endif //ENCRYPTION_H