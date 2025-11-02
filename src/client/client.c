#include "client.h"

NavigationState navigationState = USER_CREATION_MENU;

int sock;
struct sockaddr_in srv;
#define PSEUDO_MAX_LENGTH 30
char user_pseudo_buf[PSEUDO_MAX_LENGTH];

int display_color_test=0;

int main(int argc, char **argv) {
    if (argc != 3) dieNoError("Usage: COMMAND <server-ip> <port>");

    int port = atoi(argv[2]);
    const char *server_ip = argv[1];
    connect_to_server(server_ip, port, &sock, &srv);

    // begin initial messages 
    MessageUserCreation mes;
    strcpy(mes.username,"xXAnatouXx");
    sendMessageUserCreation(sock, mes);

    initApplication();
    setCursorPos(5,5);

    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock;
    pfds[1].events = POLLIN;


    while (1) {
        // drawFrameDebug();
        drawFrame();
        processEvents(pfds);
    }

    close(sock);
    return EXIT_SUCCESS;
}

void frameContent(GridCharBuffer* gcbuf) {
    char conn_infos[50];
    snprintf(conn_infos, 50, "Connecté à %s:%d", inet_ntoa(srv.sin_addr), srv.sin_port);
    drawTextWithOffset(gcbuf, conn_infos, BOTTOM_RIGHT, -1, 0);
    drawTextWithOffset(gcbuf, "Ctrl-C pour quitter", BOTTOM_LEFT, -1, 0);

    switch (navigationState)
    {
    case USER_CREATION_MENU:
        drawTitleWithOffset(gcbuf, CENTER, -10, 0);
        drawTextWithOffset(gcbuf,"Bienvenue",CENTER, 0, 0);
        drawTextWithOffset(gcbuf,"Votre pseudonyme (<ENTRÉE> pour valider):",CENTER, 2, 0);
        drawBoxWithOffset(gcbuf, 30, 1, CENTER, 4, 0);
        drawTextWithOffset(gcbuf,user_pseudo_buf,CENTER, 4, 0);
        setCursorPosRelative(gcbuf, CENTER, 4, 0);
        break;
    
    default:
        break;
    }

    if (display_color_test) {
        drawSolidRect(gcbuf, 0, 0, gcbuf->rows, gcbuf->cols, 0);
        drawDebugColors(gcbuf);
        setCursorPosRelative(gcbuf, BOTTOM_RIGHT, 0, 0);
    }
}


void processEvents(struct pollfd pfds[2]) {
    // Poll all events from STDIN and SOCK
    int sock = pfds[1].fd;
    int ready = poll(pfds, 2, -1);
    if (ready < 0) {
        if (errno == EINTR) return;
        close(sock);
        die("poll");
    }

    // stdin
    if (pfds[0].revents & POLLIN) {
        int c = terminalReadKey();
        switch (c) {
        case CTRL_KEY('c'):
            terminalClearScreen();
            exit(0);
            break;
        case CTRL_KEY('t'):
            display_color_test = ~display_color_test;
            break;
        }
    }

    // socket
    if (pfds[1].revents & POLLIN) {
        char buf[BUF_SIZE];
        ssize_t r = recv(sock, buf, sizeof(buf)-1, 0);
        if (r < 0) {
            close(sock);
            die("recv");
        } else if (r == 0) {
            close(sock);
            die("Server closed connection.\n");
        } else {
            buf[r] = '\0';
            printf("Server: %s", buf); // server may include newline
        }
    }

    if (pfds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
        die("Socket closed or error.");
    }
}

void connect_to_server(const char* server_ip, int port, int* sock_out, struct sockaddr_in* srv_out) {
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %d\n", port);
        die("Invalid port");
    }

    *sock_out = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_out < 0) die("socket"); 

    memset(srv_out, 0, sizeof(*srv_out));
    srv_out->sin_family = AF_INET;
    srv_out->sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, server_ip, &(srv_out->sin_addr)) != 1) {
        fprintf(stderr, "inet_pton failed for %s\n", server_ip);
        close(*sock_out);
        die("inet_pton failed");
    }

    if (connect(*sock_out, (struct sockaddr *)srv_out, sizeof(*srv_out)) < 0) {
        close(*sock_out);
        die("connect");
    }
    // printf("Connected to %s:%d. Type messages and press Enter.\n", server_ip, port);
}