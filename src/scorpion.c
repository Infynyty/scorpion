#include <string.h>
#include <unistd.h>
#include "Packets/Serverbound/Handshake/HandshakePacket.h"
#include "Packets/Clientbound/PacketHandler.h"
#include "Packets/Serverbound/Login/LoginStartPacket.h"
#include "Util/Logging/Logger.h"
#include "SocketWrapper.h"
#include "Packets/Serverbound/OutgoingPacket.h"


int main() {

	SocketWrapper *socket_wrapper = connect_wrapper();
	cmc_log(INFO, "Connected succesfully!\n");

	char address[] = "localhost";
    NetworkBuffer *address_buf = buffer_new();
    buffer_write(address_buf, address, strlen(address));
	HandshakePacket *handshakePacket = handshake_pkt_new(
			address_buf,
			25565,
			HANDSHAKE_STATUS
	);
    send_pkt_wrapper(&handshakePacket->_header);
    packet_free(&handshakePacket->_header);

	StatusPacket *status = status_packet_new();
	send_pkt_wrapper(&(status->_header));
	packet_free(&(status->_header));

	LoginStartPacket *loginStartPacket = login_start_packet_new();
	login_start_packet_send(loginStartPacket, socket_wrapper);
	login_start_packet_free(loginStartPacket);

	ClientState *clientState = client_state_new();
	handle_incoming_packet(socket_wrapper, clientState);



	// TODO: write exit function that frees the socket pointer and closes the socket
	close(socket_wrapper->socket);
	client_state_free(clientState);
	free(socket_wrapper);

	system("leaks scorpion");

	return 0;
}
