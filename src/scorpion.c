#include <string.h>
#include <unistd.h>
#include "Packets/Clientbound/PacketHandler.h"
#include "Packets/Serverbound/Login/LoginStartPacket.h"
#include "Util/Logging/Logger.h"
#include "SocketWrapper.h"
#include "Packets/Serverbound/Packets.h"

void print_status_response(void *packet) {
    StatusResponsePacket *response = packet;
    buffer_print_string(response->response);
}

void print_disconnect(void *packet) {
    DisconnectPlayPacket *reason = packet;
    buffer_print_string(reason->reason);
}

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
	packet_send(&handshakePacket->_header, socket_wrapper);
	packet_free(&handshakePacket->_header);

	StatusRequestPacket *status = status_request_packet_new();
	packet_send(&(status->_header), socket_wrapper);
	packet_free(&(status->_header));

    register_handler(&print_status_response, STATUS_RESPONSE_PKT);
    register_handler(&print_status_response, STATUS_RESPONSE_PKT);
    register_handler(&print_disconnect, DISCONNECT_PLAY_PKT);

	ClientState *clientState = client_state_new();
	handle_packets(socket_wrapper, clientState);


    deregister_all_handlers();

	// TODO: write exit function that frees the socket pointer and closes the socket
	close(socket_wrapper->socket);
	client_state_free(clientState);
	free(socket_wrapper);

//	system("leaks scorpion");

	return 0;
}


