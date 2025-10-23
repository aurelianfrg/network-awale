/* client.c
 *
 * Simple interactive TCP client.
 *
 * Build: gcc -o client client.c
 * Run:   ./client 127.0.0.1 12345
 *
 * Type lines and press Enter to send. Ctrl-D (EOF) to exit.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>

#include "../common/communication.h"

#define BUF_SIZE 4096

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return EXIT_FAILURE; }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, server_ip, &srv.sin_addr) != 1) {
        fprintf(stderr, "inet_pton failed for %s\n", server_ip);
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("connect");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Connected to %s:%d. Type messages and press Enter.\n", server_ip, port);


    // begin initial messages 
    MessageUserCreation mes;
    strcpy(mes.username,"xXAurelXx");
    sendMessageUserCreation(sock, mes);



    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock;
    pfds[1].events = POLLIN;

    char buf[BUF_SIZE];

    while (1) {
        int ready = poll(pfds, 2, -1);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // stdin
        if (pfds[0].revents & POLLIN) {
            ssize_t r = read(STDIN_FILENO, buf, sizeof(buf));
            if (r < 0) {
                perror("read stdin");
                break;
            } else if (r == 0) {
                // EOF from user (Ctrl-D)
                printf("EOF on stdin, closing.\n");
                break;
            } else {
                // send to server
                ssize_t sent = 0;
                while (sent < r) {
                    ssize_t s = send(sock, buf + sent, r - sent, 0);
                    if (s < 0) {
                        perror("send");
                        goto out;
                    }
                    sent += s;
                }
            }
        }

        // socket
        if (pfds[1].revents & POLLIN) {
            ssize_t r = recv(sock, buf, sizeof(buf)-1, 0);
            if (r < 0) {
                perror("recv");
                break;
            } else if (r == 0) {
                printf("Server closed connection.\n");
                break;
            } else {
                buf[r] = '\0';
                printf("Server: %s", buf); // server may include newline
            }
        }

        if (pfds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
            printf("Socket closed or error.\n");
            break;
        }
    }

out:
    close(sock);
    return EXIT_SUCCESS;
}
