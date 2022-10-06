#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <sys/types.h>

//
// Created by Kasimir on 05.10.2022.
//

int main() {
    int sockD = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(25565);
    unsigned char address[4] = {127, 0, 0, 1};
    server_address.sin_addr.S_un.S_addr = *address;


    int status = connect(sockD, (struct sockaddr*)&server_address, sizeof server_address);

    return 0;
}