//
// Created by Kasimir on 19.10.2022.
//

#ifndef CMC_CLIENTINFORMATIONPACKET_H
#define CMC_CLIENTINFORMATIONPACKET_H

#include "SocketWrapper.h"

typedef struct ClientInformationPacket ClientInformationPacket;

void client_information_packet_send(SocketWrapper *socket);


#endif //CMC_CLIENTINFORMATIONPACKET_H
