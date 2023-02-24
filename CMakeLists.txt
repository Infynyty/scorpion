cmake_minimum_required(VERSION 3.22)
project(scorpion C)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

set(CMAKE_C_STANDARD 11)

include_directories(src/Packets)
include_directories(src/State)
include_directories(src/Util)
include_directories(src/Util/ConnectionState)
include_directories(src/Util/Logging)
include_directories(src/Util/VarInt)
include_directories(src/Util/VarLong)

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
        src/Util/ConnectionState/ConnectionState.c
        src/Util/ConnectionState/ConnectionState.h
        src/Util/Logging/Logger.c
        src/Util/Logging/Logger.h
        src/Util/VarInt/MCVarInt.c
        src/Util/VarInt/MCVarInt.h
        src/Util/VarLong/VarLong.c
        src/Util/VarLong/VarLong.h
        src/Util/NBTParser.c
        src/Util/NBTParser.h
        src/Util/NetworkBuffer.c
        src/Util/NetworkBuffer.h
        src/Util/SocketWrapper.c
        src/Util/SocketWrapper.h
        src/scorpion.c src/Packets/Encryption.c src/Packets/Encryption.h src/Packets/Authentication.c src/Packets/Authentication.h)

add_compile_definitions(ACTIVE_LOG_LEVEL=INFO)

if (WIN32)
    add_subdirectory(external/curl)
    set(BUILD_CURL_EXE OFF)
    target_include_directories(scorpion PUBLIC external/curl/include)
    target_link_directories(scorpion PUBLIC external/curl/lib)
    target_link_libraries(scorpion wsock32 ws2_32)
    target_link_libraries(scorpion CURL::libcurl)
    message("${libcurl}")
else()
    find_package(CURL REQUIRED)
    target_link_libraries(scorpion CURL::libcurl)
endif()

find_package(ZLIB)
target_link_libraries(scorpion ZLIB::ZLIB)
find_package(OpenSSL REQUIRED)
target_link_libraries(scorpion OpenSSL::SSL)
find_package(json-c CONFIG)
target_link_libraries(scorpion json-c::json-c)

if (LINUX)
    target_link_libraries(scorpion m)
endif ()