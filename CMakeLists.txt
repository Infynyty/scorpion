cmake_minimum_required(VERSION 3.22)
project(CMC C)

set(CMAKE_C_STANDARD 11)

include_directories(src/Packets/Clientbound)
include_directories(src/Packets/Clientbound/Login)
include_directories(src/Packets/Clientbound/Status)
include_directories(src/Packets/Serverbound/Handshake)
include_directories(src/Packets/Serverbound/Login)
include_directories(src/Packets/Serverbound/Status)
include_directories(src/Util)
include_directories(src/Util/ConnectionState)
include_directories(src/Util/Logging)
include_directories(src/Util/VarInt)
include_directories(src/Util/VarLong)

add_executable(scorpion
        src/Packets/Clientbound/Login/DisconnectLoginPacket.c
        src/Packets/Clientbound/Login/DisconnectLoginPacket.h
        src/Packets/Clientbound/Login/LoginSuccessPacket.c
        src/Packets/Clientbound/Login/LoginSuccessPacket.h
        src/Packets/Clientbound/Login/SetCompressionPacket.c
        src/Packets/Clientbound/Login/SetCompressionPacket.h
        src/Packets/Clientbound/Status/PingResponsePacket.c
        src/Packets/Clientbound/Status/PingResponsePacket.h
        src/Packets/Clientbound/PacketHandler.c
        src/Packets/Clientbound/PacketHandler.h
        src/Util/ConnectionState/ConnectionState.c
        src/Util/ConnectionState/ConnectionState.h
        src/Util/Logging/Logger.c
        src/Util/Logging/Logger.h
        src/Util/VarInt/MCVarInt.c
        src/Util/VarInt/MCVarInt.h
        src/Util/VarLong/VarLong.c
        src/Util/VarLong/VarLong.h
        src/Util/NetworkBuffer.c
        src/Util/NetworkBuffer.h
        src/Util/SocketWrapper.c
        src/Util/SocketWrapper.h
        src/scorpion.c src/Packets/Clientbound/Play/SynchronizePlayerPositionPacket.c src/Packets/Clientbound/Play/SynchronizePlayerPositionPacket.h src/State/PlayState.c src/State/PlayState.h src/State/ClientState.c src/State/ClientState.h src/State/ServerState.c src/State/ServerState.h src/State/WorldState.c src/State/WorldState.h src/State/Position.c src/State/Position.h src/Packets/Serverbound/Play/SetPlayerPositionPacket.c src/Packets/Serverbound/Play/SetPlayerPositionPacket.h src/Packets/Serverbound/Play/SetPlayerRotationPacket.c src/Packets/Serverbound/Play/SetPlayerRotationPacket.h src/Packets/Serverbound/Play/SetPlayerPositionRotationPacket.c src/Packets/Serverbound/Play/SetPlayerPositionRotationPacket.h src/Packets/Serverbound/Packets.c src/Packets/Serverbound/Packets.h src/Util/NBTParser.c src/Util/NBTParser.h)

if (WIN32)
    target_link_libraries(scorpion wsock32 ws2_32)
endif ()

if (LINUX)
    target_link_libraries(scorpion m)
endif ()
