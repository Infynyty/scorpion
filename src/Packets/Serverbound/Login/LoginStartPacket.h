//
// Created by Kasimir on 15.10.2022.
//

#ifndef CMC_LOGINSTARTPACKET_H
#define CMC_LOGINSTARTPACKET_H


#include <winsock2.h>

typedef struct LoginStartPacket LoginStartPacket;

LoginStartPacket* login_start_packet_new();
int login_start_packet_send(LoginStartPacket* packet, SOCKET socket);
void login_start_packet_free(LoginStartPacket* packet);

#endif //CMC_LOGINSTARTPACKET_H
