//
// Created by Kasimir on 14.10.2022.
//

#ifndef CMC_CONNECTIONSTATE_H
#define CMC_CONNECTIONSTATE_H

typedef enum ConnectionState {
    HANDSHAKE,
    STATUS,
    LOGIN,
    PLAY
} ConnectionState;

void set_current_connection_state();

#endif //CMC_CONNECTIONSTATE_H
