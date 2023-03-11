cmake_minimum_required(VERSION 3.22)
project(scorpion C)

set(CMAKE_C_STANDARD 11)

include_directories(src/Packets)
include_directories(src/State)
include_directories(src/Util)
include_directories(src/Util/ConnectionState)
include_directories(src/Util/Logging)
include_directories(src/Util/VarInt)
include_directories(src/Util/VarLong)
include_directories(src/Pathfinding)
include_directories(test)

add_executable(scorpion
        src/Packets/PacketHandler.c
        src/Packets/PacketHandler.h
        src/Packets/Packets.c
        src/Packets/Packets.h
        src/Packets/Encryption.c
        src/Packets/Encryption.h
        src/State/ClientState.c
        src/State/ClientState.h
        src/State/PlayState.c
        src/State/PlayState.h
        src/State/Position.c
        src/State/Position.h
        src/State/ServerState.c
        src/State/ServerState.h
        src/State/WorldState.c
        src/State/WorldState.h
        src/Util/Logger.c
        src/Util/Logger.h
        src/Util/MCVarInt.c
        src/Util/MCVarInt.h
        src/Util/NBTParser.c
        src/Util/NBTParser.h
        src/Util/NetworkBuffer.c
        src/Util/NetworkBuffer.h
        src/Util/SocketWrapper.c
        src/Util/SocketWrapper.h
        src/scorpion.c src/Packets/Encryption.c
        src/Packets/Encryption.h src/Packets/Authentication.c
        src/Packets/Authentication.h src/Pathfinding/AStar.c
        src/Pathfinding/AStar.h src/Pathfinding/Movement.c
        src/Pathfinding/Movement.h
        test/munit.c
        test/munit.h
        test/Tests.c
        test/Tests.h)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ENABLE_TESTS)
    add_compile_definitions(ACTIVE_LOG_LEVEL=DEBUG)
endif()

find_package(CURL REQUIRED)
target_link_libraries(scorpion CURL::libcurl)
find_package(ZLIB REQUIRED)
target_link_libraries(scorpion ZLIB::ZLIB)
find_package(OpenSSL REQUIRED)
target_link_libraries(scorpion OpenSSL::SSL)
find_package(json-c REQUIRED)
target_link_libraries(scorpion json-c::json-c)
target_link_libraries(scorpion m)