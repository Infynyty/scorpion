#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "Packets/PacketHandler.h"
#include "Util/Logging/Logger.h"
#include "SocketWrapper.h"
#include "Packets/Packets.h"
#include "Authentication.h"

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

    ClientInformationPacket cip = {
            ._header = client_info_packet_new(),
            .allow_server_listings = true,
            .chat_colors = false,
            .chat_mode = varint_encode(CHAT_ENABLED),
            .displayed_skin_parts = 0,
            .enable_text_filtering = false,
            .main_hand = varint_encode(MAINHAND_RIGHT),
            .view_distance = 2,
            .locale = string_buffer_new("de_DE")
    };
	packet_send(&cip._header);
	packet_free(&cip._header);
}

void handle_init_pos(void *packet) {
	SynchronizePlayerPositionPacket *sync = packet;
	ConfirmTeleportationPacket confirmation = {
            ._header = confirm_teleportation_packet_new(),
            .teleport_id = varint_encode(varint_decode(sync->teleport_id->bytes))
    };
	packet_send(&confirmation._header);
	packet_free(&confirmation._header);
	cmc_log(INFO, "Confirmed teleportation.");

	SetPlayerPosAndRotPacket ppr = {
            ._header = set_player_pos_and_rot_packet_new(),
            sync->x + 0.2f,
            sync->y,
            sync->z,
            sync->yaw,
            sync->pitch,
            true
    };
	packet_send(&ppr._header);
	cmc_log(INFO, "Sent position.");

	ClientCommandPacket cmd = {
            ._header = client_command_packet_new(),
            .action = varint_encode(CLIENT_ACTION_RESPAWN)
    };
	packet_send(&cmd._header);
	packet_free(&cmd._header);

    cmc_log(INFO, "Spawned");

    for (int i = 0; i < 100; ++i) {
        ppr.yaw -= 5;
        packet_send(&ppr._header);
        usleep(1000 * 10);
    }

    cmc_log(INFO, "Sent all packages");

    packet_free(&ppr._header);

	double currentX = sync->y;
//    for (int i = 0; i < 100; ++i) {
	/**SetPlayerPosAndRotPacket *ppr_new = set_player_pos_and_rot_packet_new(
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
	packet_free(&ppr_new->_header);**/
//    }
}

void handle_keep_alive(void *packet) {
    cmc_log(INFO, "Received keep alive packet.");
    KeepAliveClientboundPacket *keep_alive = (KeepAliveClientboundPacket *) packet;
    KeepAliveServerboundPacket response = {
            ._header = keep_alive_serverbound_new(),
            .payload = keep_alive->payload
    };
    packet_send(&response._header);
    packet_free(&response._header);
}

void handle_encryption(void *packet) {
	cmc_log(INFO, "Encryption request received");
}

void get_status() {
	char address[] = "localhost";
	NetworkBuffer *address_buf = buffer_new();
	buffer_write_little_endian(address_buf, address, strlen(address));
	HandshakePacket *handshakePacket = handshake_pkt_header();
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

    authenticate_xbl();

	SocketWrapper *socket_wrapper = connect_wrapper();
	cmc_log(INFO, "Connected succesfully!");


	char address[] = "localhost";
	NetworkBuffer *address_buf = buffer_new();
	buffer_write_little_endian(address_buf, address, strlen(address));
	HandshakePacket handshakePacket = {
            ._header = handshake_pkt_header(),
            .protocol_version = varint_encode(761),
            .port = 25565,
            .address = address_buf,
            .next_state = varint_encode(2)
    };
	packet_send(&handshakePacket._header);
	packet_free(&handshakePacket._header);

	NetworkBuffer *uuid = buffer_new();
	uint64_t low = 0;
	uint64_t high = 0;
	buffer_write(uuid, &low, sizeof(uint64_t));
	buffer_write(uuid, &high, sizeof(uint64_t));
    LoginStartPacket start = {
            ._header = login_start_packet_header(),
            .player_name = string_buffer_new("Infy"),
            .has_player_uuid = true,
            .uuid = uuid
    };
    packet_send(&start._header);
    packet_free(&start._header);

	cmc_log(INFO, "Login start");


	register_handler(&print_disconnect, DISCONNECT_PLAY_PKT);
	register_handler(&print_disconnect_login, LOGIN_DISCONNECT_PKT);
	register_handler(&handle_login_success, LOGIN_SUCCESS_PKT);
	register_handler(&handle_login_play, LOGIN_PLAY_PKT);
	register_handler(&handle_init_pos, SYNCHRONIZE_PLAYER_POS_PKT);
	register_handler(&handle_keep_alive, KEEP_ALIVE_CLIENTBOUND_PKT);


	ClientState *clientState = client_state_new();
	handle_packets(socket_wrapper, clientState);


	deregister_all_handlers();

	// TODO: write exit function that frees the socket pointer and closes the socket
	close(socket_wrapper->socket);
	client_state_free(clientState);
	free(socket_wrapper);

    //system("export MallocStackLogging=1");
    //system("leaks scorpion");

	return 0;
}


