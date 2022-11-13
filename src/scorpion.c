#include <string.h>
#include <unistd.h>
#include "Packets/Clientbound/PacketHandler.h"
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
            writeVarInt(ENABLED),
            true,
            0,
            writeVarInt(MAINHAND_RIGHT),
            false,
            true
            );
    packet_send(&cip->_header, get_socket());
    packet_free(&cip->_header);
}

void handle_init_pos(void *packet) {
    SynchronizePlayerPositionPacket *sync = packet;
    MCVarInt *teleport_id = writeVarInt(varint_decode(sync->teleport_id));
    ConfirmTeleportationPacket *confirmation = confirm_teleportation_packet_new(teleport_id);
    packet_send(&confirmation->_header, get_socket());
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
    packet_send(&ppr->_header, get_socket());
    cmc_log(INFO, "Sent position.");

    ClientCommandPacket *cmd = client_command_packet_new(writeVarInt(CLIENT_ACTION_RESPAWN));
    packet_send(&cmd->_header, get_socket());
    packet_free(&cmd->_header);

    usleep(100*1000);
    ppr->x = sync->x + 0.5f;
    packet_send(&ppr->_header, get_socket());
    packet_free(&ppr->_header);
}

int main() {
//    int test = 0x6eba5;
//    int decoded = varint_decode((unsigned char *) &test);
//
//    exit(EXIT_SUCCESS);

	SocketWrapper *socket_wrapper = connect_wrapper();
	cmc_log(INFO, "Connected succesfully!");

	char address[] = "localhost";
	NetworkBuffer *address_buf = buffer_new();
	buffer_write(address_buf, address, strlen(address));
	HandshakePacket *handshakePacket = handshake_pkt_new(
			address_buf,
			25565,
			HANDSHAKE_LOGIN
	);
	packet_send(&handshakePacket->_header, socket_wrapper);
	packet_free(&handshakePacket->_header);

	LoginStartPacket *loginStartPacket = login_start_packet_new(string_buffer_new("Infy"), true);
    packet_send(&loginStartPacket->_header, socket_wrapper);
    packet_free(&loginStartPacket->_header);


    register_handler(&print_status_response, STATUS_RESPONSE_PKT);
    register_handler(&print_disconnect, DISCONNECT_PLAY_PKT);
    register_handler(&handle_login_success, LOGIN_SUCCESS);
    register_handler(&handle_login_play, LOGIN_PLAY);
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


