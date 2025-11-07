/* server.c
 *
 * Simple multi-client echo server using poll().
 *
 * Build: gcc -o server server.c
 * Run:   ./server 12345
 *
 * Author: example
 */

#include "server.h"

#define BACKLOG 16
#define MAX_CLIENTS 1024
#define BUF_SIZE 4096

// GAME LOGIC
User* users[MAX_CLIENTS];


// CONNECTION LOGIC
static volatile sig_atomic_t keep_running = 1;

void int_handler(int _) { (void)_; keep_running = 0; }

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (set_nonblocking(listen_fd) < 0) {
        perror("set_nonblocking");
        // not fatal; continue
    }

    struct pollfd *pfds = calloc(MAX_CLIENTS + 1, sizeof(struct pollfd));
    if (!pfds) {
        perror("calloc");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    pfds[0].fd = listen_fd;
    pfds[0].events = POLLIN;
    int nfds = 1; // number of used entries in pfds

    printf("Server listening on port %d\n", port);

    char buf[BUF_SIZE];

    while (keep_running) {
        int timeout_ms = 1000; // wakeup every second to check signal
        int ready = poll(pfds, nfds, timeout_ms);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        } else if (ready == 0) {
            continue; // timeout, loop again to check keep_running
        }

        // Check listening socket
        if (pfds[0].revents & POLLIN) {
            // accept in a loop (because non-blocking)
            while (1) {
                struct sockaddr_in cli_addr;
                socklen_t cli_len = sizeof(cli_addr);
                int client_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
                if (client_fd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    perror("accept");
                    break;
                }

                // find free slot
                if (nfds >= MAX_CLIENTS + 1) {
                    fprintf(stderr, "Too many connections, rejecting\n");
                    close(client_fd);
                    continue;
                }

                if (set_nonblocking(client_fd) < 0) {
                    // not fatal
                }

                pfds[nfds].fd = client_fd;
                pfds[nfds].events = POLLIN;
                nfds++;

                char ipbuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cli_addr.sin_addr, ipbuf, sizeof(ipbuf));
                printf("Accepted %s:%d (fd=%d)\n", ipbuf, ntohs(cli_addr.sin_port), client_fd);
            }
        }

        // Check client sockets (start at 1)
        for (int i = 1; i < nfds; ++i) {
            int fd = pfds[i].fd;
            short re = pfds[i].revents;
            if (re == 0) continue;

            if (re & (POLLERR | POLLHUP | POLLNVAL)) {
                // client disconnected/error
                close(fd);
                // remove this pfds entry by shifting
                pfds[i] = pfds[nfds - 1];
                nfds--;
                i--; // check the moved one on next iteration
                continue;
            }

            if (re & POLLIN) {
                ssize_t r = recv(fd, buf, sizeof(buf), 0);
                if (r < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    perror("recv");
                    close(fd);
                    pfds[i] = pfds[nfds - 1];
                    nfds--;
                    i--;
                    continue;
                } else if (r == 0) {
                    // client closed
                    printf("Client fd=%d closed\n", fd);
                    close(fd);
                    pfds[i] = pfds[nfds - 1];
                    nfds--;
                    i--;
                    continue;
                } else {

                    // index user using fd, which identifies it and never changes
                    if (fd >= MAX_CLIENTS) {
                        printf("error : a file descriptor is higher than the max number of clients");
                    }
                    User* current_user = users[fd]; // user from whom we received a message

                    // standard case : process how you handle the client when a message was received
                    printf("\nMessage length : %lu\n",r);
                    int message_type;
                    message_type = *(int*)buf;

                    MessageUserCreation userCreationMes;
                    if (message_type == USER_CREATION) {
                        userCreationMes = * (MessageUserCreation*) (buf+sizeof(int));
                        printf("message type : %d\n", message_type);
                        printf("username : %s\n", userCreationMes.username);
                    }

                     
                     
                    // standard case : process how you handle the client
                    // ssize_t sent = 0;
                    // while (sent < r) {
                    //     ssize_t s = send(fd, buf + sent, r - sent, 0);
                    //     if (s < 0) {
                    //         if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    //         perror("send");
                    //         break;
                    //     }
                    //     sent += s;
                    // }
                    // // For visibility, print what we got (trim newline)
                    // printf("fd=%d: %.*s\n", fd, (int)r, (int)r ? buf : "(empty)");
                }
            }
        }
    }

    printf("Shutting down server...\n");
    // Close all open fds
    for (int i = 0; i < nfds; ++i) close(pfds[i].fd);
    free(pfds);
    close(listen_fd);

    return EXIT_SUCCESS;
}

//int handle_message(int message_type, User* source_user);
