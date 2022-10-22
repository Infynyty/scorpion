//
// Created by Kasimir Stadie on 21.10.22.
//

#include <stdbool.h>
#include "SynchronizePlayerPositionPacket.h"
#include "../../../State/PlayState.h"
#include "NetworkBuffer.h"

void synchronize_player_position_packet_handle(SocketWrapper *socketWrapper, ClientState *clientState) {
    double x = buffer_receive_double(socketWrapper);
    double y = buffer_receive_double(socketWrapper);
    double z = buffer_receive_double(socketWrapper);
    float yaw = buffer_receive_float(socketWrapper);
    float pitch = buffer_receive_float(socketWrapper);
    uint8_t flags = buffer_receive_uint8_t(socketWrapper); //TODO: implement
    int teleportID = varint_receive(socketWrapper);
    bool dismountVehicle = buffer_receive_uint8_t(socketWrapper); //TODO: implement

    Position *position = position_new(x, y, z, yaw, pitch);
    client_update_position(clientState, position);
    client_set_teleportID(clientState, teleportID);
}
