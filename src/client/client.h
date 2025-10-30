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

#include "tui.h"
#include "../common/communication.h"

#define BUF_SIZE 4096

typedef enum NavigationState {
    USER_CREATION_MENU,
    MAIN_MENU,
    IN_QUEUE_MENU,
    GAME_START_MENU,
    IN_GAME_MENU,
    GAME_END_MENU
} NavigationState;


void connect_to_server(const char* server_ip, int port, int* sock_out, struct sockaddr_in* srv_out);
void processEvents(struct pollfd pfds[2]);