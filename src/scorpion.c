#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "Packets/PacketHandler.h"
#include "Util/Logging/Logger.h"
#include "SocketWrapper.h"
#include "Packets/Packets.h"
#include "Authentication.h"
#include "WorldState.h"
#include "munit.h"
#include <openssl/bn.h>
#include "Tests.h"

void print_status_response(void *packet, PlayState *state) {
	StatusResponsePacket *response = packet;
	buffer_print_string(response->response);
}

void print_disconnect(void *packet, PlayState *state) {
	DisconnectPlayPacket *reason = packet;
	cmc_log(INFO, "Disconnected: %s", reason->reason->bytes);
}

void print_disconnect_login(void *packet, PlayState *state) {
	DisconnectLoginPacket *reason = packet;
	cmc_log(ERR, "Disconnected during login with the following message: %s", reason->reason->bytes);
}

void handle_login_success(void *packet, PlayState *state) {
	LoginSuccessPacket *success = packet;
	cmc_log(INFO, "Logged in with username %s.", success->username->bytes);
}

void handle_login_play(void *packet, PlayState *state) {
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

void handle_init_pos(void *packet, PlayState *state) {
	SynchronizePlayerPositionPacket *sync = packet;
	ConfirmTeleportationPacket confirmation = {
            ._header = confirm_teleportation_packet_new(),
            .teleport_id = varint_encode(varint_decode(sync->teleport_id->bytes))
    };
	packet_send(&confirmation._header);
	packet_free(&confirmation._header);
	cmc_log(INFO, "Confirmed teleportation.");
    state->clientState->position = position_new(sync->x, sync->y, sync->z, sync->yaw, sync->pitch);

	SetPlayerPosAndRotPacket ppr = {
            ._header = set_player_pos_and_rot_packet_new(),
            sync->x,
            sync->y,
            sync->z,
            sync->yaw,
            sync->pitch + 1,
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
    packet_free(&ppr._header);
}

void handle_update_chunk(void *packet, PlayState *state) {
    ChunkDataPacket *data = (ChunkDataPacket *) packet;
    add_chunk(data, state->worldState);
    cmc_log(INFO, "Updated chunk at %d, %d", data->chunk_x, data->chunk_z);
}

void handle_remove_chunk(void *packet, PlayState *state) {
    UnloadChunkPacket *data = (UnloadChunkPacket *) packet;
    remove_chunk(data, state->worldState);
}

void handle_keep_alive(void *packet, PlayState *state) {
    cmc_log(INFO, "Received keep alive packet.");
    KeepAliveClientboundPacket *keep_alive = (KeepAliveClientboundPacket *) packet;
    KeepAliveServerboundPacket response = {
            ._header = keep_alive_serverbound_new(),
            .payload = keep_alive->payload
    };
    packet_send(&response._header);
    packet_free(&response._header);
}

void handle_encryption(void *packet, PlayState *state) {
	cmc_log(INFO, "Encryption request received");
}

void get_status() {
	char address[] = "localhost";
	NetworkBuffer *address_buf = buffer_new();
    buffer_write(address_buf, address, strlen(address));
	HandshakePacket *handshakePacket = handshake_pkt_header();
	packet_send(&handshakePacket->_header);
	packet_free(&handshakePacket->_header);

	StatusRequestPacket *request = status_request_packet_new();
	packet_send(&request->_header);
	packet_free(&request->_header);

	register_handler(&print_status_response, STATUS_RESPONSE_PKT);

	ClientState *clientState = client_state_new();


}

void handle_login(ClientState *client) {
    char address[] = "localhost";
    NetworkBuffer *address_buf = buffer_new();
    buffer_write(address_buf, address, strlen(address));
    HandshakePacket handshakePacket = {
            ._header = handshake_pkt_header(),
            .protocol_version = varint_encode(761),
            .port = 25565,
            .address = address_buf,
            .next_state = varint_encode(2)
    };
    packet_send(&handshakePacket._header);
    packet_free(&handshakePacket._header);

    BIGNUM *bn = BN_new();
    NetworkBuffer *uuid_bn = buffer_clone(client->profile_info->name);
    buffer_write(uuid_bn, "\0", 1);
    BN_hex2bn(&bn, uuid_bn->bytes);
    char *temp = malloc(16);
    BN_bn2bin(bn, temp);
    buffer_remove(uuid_bn, uuid_bn->size);
    buffer_write(uuid_bn, temp, 16);

    LoginStartPacket start = {
            ._header = login_start_packet_header(),
            .player_name = buffer_clone(client->profile_info->name),
            .has_player_uuid = true,
            .uuid = uuid_bn
    };
    packet_send(&start._header);
    packet_free(&start._header);
    free(temp);
    BN_free(bn);

    cmc_log(INFO, "Sent login data for player %s with UUID %.32s.",
            client->profile_info->name->bytes,
            client->profile_info->uuid->bytes
            );
}

void register_handlers() {
    register_handler(&print_disconnect, DISCONNECT_PLAY_PKT);
    register_handler(&print_disconnect_login, LOGIN_DISCONNECT_PKT);
    register_handler(&handle_login_success, LOGIN_SUCCESS_PKT);
    register_handler(&handle_login_play, LOGIN_PLAY_PKT);
    register_handler(&handle_init_pos, SYNCHRONIZE_PLAYER_POS_PKT);
    register_handler(&handle_keep_alive, KEEP_ALIVE_CLIENTBOUND_PKT);
    register_handler(&handle_update_chunk, CHUNK_DATA_PKT);
    register_handler(&handle_remove_chunk, UNLOAD_CHUNK_PKT);
}


int main() {
    run_tests();

    PlayState *play_state = play_state_new();
    cmc_log(DEBUG, "Initializing global palette...");
    init_global_palette(play_state->worldState);
    cmc_log(DEBUG, "Successfully initialized global palette.");

    authenticate(play_state->clientState);

    cmc_log(DEBUG, "Trying to connect to server socket...");
	SocketWrapper *socket_wrapper = connect_wrapper();
	cmc_log(DEBUG, "Connection to server socket was successful.");


    register_handlers();
    cmc_log(DEBUG, "Registered handlers.");
    handle_login(play_state->clientState);
    cmc_log(DEBUG, "Sent login packets.");
    cmc_log(INFO, "Ready to receive packets.");

    pthread_t thread_receive;
    pthread_t thread_handle;

    pthread_cond_t condition;
    pthread_cond_init(&condition, NULL);

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    GenericPacketList list = {.first = NULL, .last = NULL, .mutex = &mutex, .condition = &condition};
    PacketHandleWrapper wrapper = {.state = play_state, .list = &list, .receive_thread = &thread_receive};

    pthread_create(&thread_handle, NULL, handle_packets, &wrapper);

    pthread_join(thread_handle, NULL);
    pthread_cancel(thread_receive);

    pthread_cond_destroy(&condition);

    cmc_log(INFO, "Disconnecting...");
	deregister_all_handlers();
    cmc_log(DEBUG, "Deregistered all handlers.");
    GenericPacket *to_free = list.first;
    while (to_free != list.last) {
        GenericPacket *temp = to_free->next;
        generic_packet_free(to_free);
        to_free = temp;
    }
	close(socket_wrapper->socket);
    play_state_free(play_state);
	free(socket_wrapper);

    cmc_log(INFO, "Disconnected successfully.");

    //system("export MallocStackLogging=1");
    //system("leaks scorpion");

	return 0;
}


