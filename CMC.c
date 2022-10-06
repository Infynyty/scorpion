#include "Dog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <Winsock2.h>
#endif

#ifdef __APPLE__
#include <netinet/in.h>
#endif

int main() {
    int sockD = socket( PF_LOCAL, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    uint32_t ip_address = 2130706433; // 127.0.0.1 as an integer
    server_address.sin_addr.s_addr = ip_address;
    server_address.sin_port = 25565;
    server_address.sin_family = AF_INET;
    printf("Trying to connect to port %d.\n", server_address.sin_port);
    int connection_status = connect(sockD, (const struct sockaddr *) &server_address, sizeof server_address);

    if (connection_status == -1) {
        printf("Fehler!");
        return -1;
    }

    close(connection_status);

    return 0;
}
