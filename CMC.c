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


#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }
    SOCKET sockD = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
#ifdef __APPLE__
    int sockD = socket( PF_LOCAL, SOCK_STREAM, 0);
#endif
    char address[] = "localhost";
    Header* header = header_new(address, strlen(address), 25565);

    struct sockaddr_in server_address;
//    uint32_t ip_address = 2130706433; // 127.0.0.1 as an integer
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(25565);
    server_address.sin_family = AF_INET;

    printf("Trying to connect to port %d.\n", server_address.sin_port);
    int connection_status = connect(sockD, (const struct sockaddr *) &server_address, sizeof server_address);

    if (connection_status == -1) {
        printf("Fehler!");
        return -1;
    }
    printf("Connected succesfully!\n");

    char emptyPacket = 0x00;
    char ping = 0x01;
    char* test = calloc(8, sizeof(char));
    Buffer* buffer = get_ptr_buffer(header);
    buffer_send(buffer, sockD);
    buffer_free(buffer);

    send(sockD, &emptyPacket, 1, 0);
    send(sockD, &ping, 1, 0);
    send(sockD, test, sizeof(long), 0);

    char* answer = calloc(32767, sizeof(char));
    int packets = recv(sockD, answer, 2, 0);
    for (int i = 0; i < packets; ++i) {
        printf("%c", answer[i]);
    }

    printf("\n Packets received: %d", packets);

    close(connection_status);

    return 0;
}
