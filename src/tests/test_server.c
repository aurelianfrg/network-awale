#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../common/game.h"
#include "../common/communication.h"

// executable test for when the server is running


void connect_to_server(const char* server_ip, int port, int* sock_out, struct sockaddr_in* srv_out) {
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %d\n", port);
        exit(-1);
    }

    *sock_out = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_out < 0) exit(-1);

    memset(srv_out, 0, sizeof(*srv_out));
    srv_out->sin_family = AF_INET;
    srv_out->sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, server_ip, &(srv_out->sin_addr)) != 1) {
        fprintf(stderr, "inet_pton failed for %s\n", server_ip);
        close(*sock_out);
        exit(-1);
    }

    if (connect(*sock_out, (struct sockaddr *)srv_out, sizeof(*srv_out)) < 0) {
        close(*sock_out);
        exit(-1);
    }
    printf("Connected to %s:%d.\n", server_ip, port);
}


int main(int argc, char **argv) {

    if (argc != 3) {
        printf("Usage: COMMAND <server-ip> <port>\n");
        exit(-1);
    }

    // NETWORKING
    int sock;
    struct sockaddr_in srv;
    int port = atoi(argv[2]);
    const char *server_ip = argv[1];
    connect_to_server(server_ip, port, &sock, &srv);

    // TESTS


    return 0;
}