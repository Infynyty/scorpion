#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
//#include <stdint.h>
#include "Packets/VarInt/VarInt.h"

#include "Packets/Packet.h"

#ifdef _WIN32
//    #include <winsock2.h>
#endif

#ifdef __APPLE__
    #include <netinet/in.h>
#endif


int main() {
    VarInt* varInt = writeVarInt(255);
    char array[get_length(varInt)];
    memcpy(&array, get_bytes(varInt), get_length(varInt));

    for (int i = 0; i < get_length(varInt); ++i) {
        printf("\nNumber in bytes: %d", get_bytes(varInt)[i]);
        printf("\nNumber: %d", array[i]);
    }



//    int sockD;
//#ifdef _WIN32
//    WSADATA wsaData;
//    if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
//        fprintf(stderr, "WSAStartup failed.\n");
//        exit(1);
//    }
//    sockD = socket( AF_INET, SOCK_STREAM, 0);
//#endif
//#ifdef __APPLE__
//    sockD = socket( PF_LOCAL, SOCK_STREAM, 0);
//#endif
//    char ip_address[255] = {0};
//    ip_address[0] = 127;
//    ip_address[3] = 1;
//    for (int i = 0; i < 256; i++)
//        printf("%d", ip_address[i]);
//    Header* header = header_new(ip_address, 25565);
//    print_header(header);
//
//    struct sockaddr_in server_address;
//    //uint32_t ip_address = 2130706433; // 127.0.0.1 as an integer
//    server_address.sin_addr.s_addr = ip_address;
//    server_address.sin_port = 25565;
//    server_address.sin_family = AF_INET;
//
//    printf("Trying to connect to port %d.\n", server_address.sin_port);
//    int connection_status = connect(sockD, (const struct sockaddr *) &server_address, sizeof server_address);
//
//    if (connection_status == -1) {
//        printf("Fehler!");
//        return -1;
//    }
//
//    close(connection_status);

    return 0;
}
