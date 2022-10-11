#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "Packets/Packet.h"

#ifdef _WIN32
    #include <winsock2.h>
#endif

#ifdef __APPLE__
    #include <netinet/in.h>
#endif


int main() {



    int sockD;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }
    sockD = socket( AF_INET, SOCK_STREAM, 0);
#endif
#ifdef __APPLE__
    sockD = socket( PF_LOCAL, SOCK_STREAM, 0);
#endif
    char ip_address[255] = {0};
    ip_address[0] = 127;
    ip_address[3] = 1;
    Header* header = header_new(ip_address, 25565);

    struct sockaddr_in server_address;
//    uint32_t ip_address = 2130706433; // 127.0.0.1 as an integer
    server_address.sin_addr.s_addr = ip_address;
    server_address.sin_port = 25565;
    server_address.sin_family = AF_INET;

    printf("Trying to connect to port %d.\n", server_address.sin_port);
    int connection_status = connect(sockD, (const struct sockaddr *) &server_address, sizeof server_address);

    if (connection_status == -1) {
        printf("Fehler!");
        return -1;
    }

    send(sockD, get_ptr_buffer(header), get_header_size(header), 0);
    char answer[32767] = {0};
    read(sockD, &answer, 32767);
    for (int i = 0; i < 32767; ++i) {
        printf("%c", answer[i]);
    }

    close(connection_status);

    return 0;
}
