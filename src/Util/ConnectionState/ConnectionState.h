#ifndef CMC_CONNECTIONSTATE_H
#define CMC_CONNECTIONSTATE_H

typedef enum ConnectionState {
	HANDSHAKE,
	STATUS,
	LOGIN,
	PLAY
} ConnectionState;

typedef enum PacketDirection {
	SERVERBOUND,
	CLIENTBOUND
} PacketDirection;

void set_current_connection_state();

#endif //CMC_CONNECTIONSTATE_H
