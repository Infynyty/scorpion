# scorpion

scorpion is a custom client for the game Minecraft written completely in C. It is currently in development. 

## Compatability

scorpion is written for Minecraft version 1.19.3. It currently compiles using macOS and Linux. 

## Installation

To install scorpion, use CMake. scorpion requires the following packages to compile:

- CURL
- ZLIB
- OpenSSL
- json-c

Compilation using CMake, gcc and Ninja might look something like this:

``cmake -DCMAKE_BUILD_TYPE=Debug DCMAKE_MAKE_PROGRAM=path/to/ninja -DCMAKE_C_COMPILER=path/to/gcc -G Ninja -S path/to/CMakeLists.txt -B path/to/output``

## Features

Current features include:

- Implementation of all status and login packets. Implementation of a few important play packets.
- Packet event handler system
- Structures to save important data about the world, player and server
- Processing chunk data
- Packet encryption and compression
- Joining online servers using the Microsoft Authentication scheme

## Planned features

- Pathfinding using A*
- Automating tasks like mining
- Multithreading for receiving packets and handling chunk data

## Current issues

- No input sanitization for packet data
