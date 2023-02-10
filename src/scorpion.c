#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "Packets/PacketHandler.h"
#include "Util/Logging/Logger.h"
#include "SocketWrapper.h"
#include "Packets/Packets.h"

void print_status_response(void *packet) {
	StatusResponsePacket *response = packet;
	buffer_print_string(response->response);
}

void print_disconnect(void *packet) {
	DisconnectPlayPacket *reason = packet;
	cmc_log(INFO, "Disconnected: %s", reason->reason->bytes);
}

void print_disconnect_login(void *packet) {
	DisconnectLoginPacket *reason = packet;
	cmc_log(INFO, "Disconnected: %s", reason->reason->bytes);
}

void handle_login_success(void *packet) {
	LoginSuccessPacket *success = packet;
	cmc_log(INFO, "Logged in with username %s.", success->username->bytes);
}

void handle_login_play(void *packet) {
	LoginPlayPacket *play = packet;
	cmc_log(INFO, "Logged in with gamemode %d.", play->gamemode);

	ClientInformationPacket *cip = client_info_packet_new(
			string_buffer_new("de_DE"),
			5,
			varint_encode(ENABLED),
			true,
			0,
			varint_encode(MAINHAND_RIGHT),
			false,
			true
	);
	packet_send(&cip->_header);
	packet_free(&cip->_header);
}

void handle_init_pos(void *packet) {
	SynchronizePlayerPositionPacket *sync = packet;
	MCVarInt *teleport_id = varint_encode(varint_decode(sync->teleport_id));
	ConfirmTeleportationPacket *confirmation = confirm_teleportation_packet_new(teleport_id);
	packet_send(&confirmation->_header);
	packet_free(&confirmation->_header);
	cmc_log(INFO, "Confirmed teleportation.");

	SetPlayerPosAndRotPacket *ppr = set_player_pos_and_rot_packet_new(
			sync->x + 0.05,
			sync->y,
			sync->z,
			sync->yaw,
			sync->pitch,
			true
	);
	packet_send(&ppr->_header);
	packet_free(&ppr->_header);
	cmc_log(INFO, "Sent position.");

	ClientCommandPacket *cmd = client_command_packet_new(varint_encode(CLIENT_ACTION_RESPAWN));
	packet_send(&cmd->_header);
	packet_free(&cmd->_header);

	double currentX = sync->y;
//    for (int i = 0; i < 100; ++i) {
	SetPlayerPosAndRotPacket *ppr_new = set_player_pos_and_rot_packet_new(
			sync->x + 0.05,
			sync->y,
			sync->z,
			sync->yaw,
			sync->pitch,
			true
	);
	currentX += -100.0f;
	ppr_new->y = currentX;
	cmc_log(DEBUG, "Position x: %lf", currentX);
	cmc_log(DEBUG, "Sending packet x: %lf", currentX);
	packet_send(&ppr_new->_header);
	packet_free(&ppr_new->_header);
//    }

}

void handle_encryption(void *packet) {
	cmc_log(INFO, "Encryption request received");
}

void get_status() {
	char address[] = "localhost";
	NetworkBuffer *address_buf = buffer_new();
	buffer_write_little_endian(address_buf, address, strlen(address));
	HandshakePacket *handshakePacket = handshake_pkt_new(
			address_buf,
			25565,
			HANDSHAKE_STATUS
	);
	packet_send(&handshakePacket->_header);
	packet_free(&handshakePacket->_header);

	StatusRequestPacket *request = status_request_packet_new();
	packet_send(&request->_header);
	packet_free(&request->_header);

	register_handler(&print_status_response, STATUS_RESPONSE_PKT);

	ClientState *clientState = client_state_new();
	handle_packets(get_socket(), clientState);

}


int main() {

	SocketWrapper *socket_wrapper = connect_wrapper();
	cmc_log(INFO, "Connected succesfully!");


	char address[] = "localhost";
	NetworkBuffer *address_buf = buffer_new();
	buffer_write_little_endian(address_buf, address, strlen(address));
	HandshakePacket *handshakePacket = handshake_pkt_new(
			address_buf,
			25565,
			HANDSHAKE_LOGIN
	);
	packet_send(&handshakePacket->_header);
	packet_free(&handshakePacket->_header);

	NetworkBuffer *uuid = buffer_new();
	uint64_t low = 0;
	uint64_t high = 0;
	buffer_write(uuid, &low, sizeof(uint64_t));
	buffer_write(uuid, &high, sizeof(uint64_t));
	LoginStartPacket *loginStartPacket = login_start_packet_new(string_buffer_new("Infy"), true, uuid);
	packet_send(&loginStartPacket->_header);
	packet_free(&loginStartPacket->_header);

	cmc_log(INFO, "Login start");


	register_handler(&print_disconnect, DISCONNECT_PLAY_PKT);
	register_handler(&print_disconnect_login, LOGIN_DISCONNECT_PKT);
	register_handler(&handle_login_success, LOGIN_SUCCESS_PKT);
	register_handler(&handle_login_play, LOGIN_PLAY_PKT);
	register_handler(&handle_init_pos, SYNCHRONIZE_PLAYER_POS_PKT);


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


