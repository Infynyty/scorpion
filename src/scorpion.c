#include <string.h>
#include <unistd.h>
#include "Packets/Serverbound/Handshake/HandshakePacket.h"
#include "Packets/Clientbound/PacketHandler.h"
#include "Packets/Serverbound/Login/LoginStartPacket.h"
#include "Util/Logging/Logger.h"
#include "SocketWrapper.h"


int main() {

    SocketWrapper* socket_wrapper = connect_wrapper();
    cmc_log(INFO, "Connected succesfully!\n");

    char address[] = "localhost";
    HandshakePacket* handshakePacket = handshake_packet_new(
            address,
            strlen(address),
            25565,
            HANDSHAKE_NEXT_STATE_LOGIN
            );
    handshake_packet_send(handshakePacket, socket_wrapper);
    handshake_packet_free(handshakePacket);

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
