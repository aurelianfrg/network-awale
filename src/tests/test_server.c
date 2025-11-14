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


void connect_to_server(const char* server_ip, int32_t port, int32_t* sock_out, struct sockaddr_in* srv_out) {
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
    int32_t sock;
    struct sockaddr_in srv;
    int32_t port = atoi(argv[2]);
    const char *server_ip = argv[1];
    printf("before connection.\n");
    connect_to_server(server_ip, port, &sock, &srv);
    printf("first connection successful.\n");


    // TESTS

    // user creation
    MessageUserCreation user_creation_msg;
    strcpy(user_creation_msg.username, "Aurelll");
    sendMessageUserCreation(sock, user_creation_msg);

    // get response from server
    int32_t message_type;
    recv(sock, &message_type, sizeof(int32_t), 0);
    if (message_type == USER_REGISTRATION) {
        MessageUserRegistration msg;
        recv(sock, &msg, sizeof(msg), 0);
        printf("Server acknowledged user registration with id %d.\n", msg.user_id);
    } 
    else {
        printf("Server did not acknowledge user registration.\n");
    }

    // second creation that should fail
    sleep(1);
    sendMessageUserCreation(sock, user_creation_msg);


    // init a second connection
    int32_t sock2;
    struct sockaddr_in srv2;
    connect_to_server(server_ip, port, &sock2, &srv2);
    // register the second player
    MessageUserCreation user_creation_msg2;
    strcpy(user_creation_msg2.username, "Anatouuu");
    sendMessageUserCreation(sock2, user_creation_msg2);
    recv(sock2, &message_type, sizeof(int32_t), 0);
    if (message_type == USER_REGISTRATION) {
        MessageUserRegistration msg;
        recv(sock2, &msg, sizeof(msg), 0);
        printf("Server acknowledged user registration with id %d.\n", msg.user_id);
    } 
    else {
        printf("Server did not acknowledge user registration.\n");
    }

    // init a third connection
    int32_t sock3;
    struct sockaddr_in srv3;
    connect_to_server(server_ip, port, &sock3, &srv3);
    // register the third player
    MessageUserCreation user_creation_msg3;
    strcpy(user_creation_msg3.username, "autre");
    sendMessageUserCreation(sock3, user_creation_msg3);
    recv(sock3, &message_type, sizeof(int32_t), 0);
    if (message_type == USER_REGISTRATION) {
        MessageUserRegistration msg;
        recv(sock3, &msg, sizeof(msg), 0);
        printf("Server acknowledged user registration with id %d.\n", msg.user_id);
    } 
    else {
        printf("Server did not acknowledge user registration.\n");
    }

    // get users list
    sendMessageGetUserList(sock);
    // hopefully get server response
    recv(sock, &message_type, sizeof(int32_t), 0);
    if (message_type == SEND_USER_LIST) {
        int32_t usernames_count;
        recv(sock, &usernames_count, sizeof(int32_t), 0);
        char buffer[USERNAME_LENGTH];
        printf("Users : \n");
        for (int i = 0; i < usernames_count; ++i) {
            recv(sock, &buffer, sizeof(char)*USERNAME_LENGTH, 0);
            printf(" - %s\n", buffer);
        }
        int32_t id;
        printf("Corresponding ids : \n");
        for (int i = 0; i < usernames_count; ++i) {
            recv(sock, &id, sizeof(int32_t), 0);
            printf(" - %d\n", id);
        }
    }
    
    // try to create a game
    MessageMatchRequest req;
    req.opponent_id = 1;
    sendMessageMatchRequest(sock, req);
    printf("sending match request from 0 to 1.\n");

    recv(sock2, &message_type, sizeof(int32_t), 0);
    if (message_type == MATCH_PROPOSITION) {
        printf("2 received match proposition.\n");
        MessageMatchProposition invite_msg;
        recv(sock2, &invite_msg, sizeof(invite_msg), 0);
        printf("it is from %s.\n", invite_msg.opponent_username);
    }

    // // user 1 cancels game creation
    // printf("user 1 cancels game.\n");
    // sendMessageMatchCancellation(sock);

    // sleep(1);

    // // user 2 accepts game creation
    // sendMessageMatchResponse(sock2, true);

    // sleep(1);

    // // try to create a game again
    // req.opponent_id = 1;
    // sendMessageMatchRequest(sock, req);
    // printf("sending match request from 0 to 1.\n");

    // recv(sock2, &message_type, sizeof(int32_t), 0);
    // if (message_type == MATCH_PROPOSITION) {
    //     printf("2 received match proposition.\n");
    //     MessageMatchProposition invite_msg;
    //     recv(sock2, &invite_msg, sizeof(invite_msg), 0);
    //     printf("it is from %s.\n", invite_msg.opponent_username);
    // }

    sleep(1);
    
    // user 2 accepts game creation
    sendMessageMatchResponse(sock2, true);

    recv(sock, &message_type, sizeof(int32_t), 0);
    MessageGameStart start_mes;
    if (message_type == GAME_START) {
        printf("player 0 received game start message.\n");
        recv(sock, &start_mes, sizeof(start_mes), 0);
    }
    else { printf("error: unexpected message %d\n", message_type); }

    recv(sock2, &message_type, sizeof(int32_t), 0);
    if (message_type == GAME_START) {
        printf("player 1 received game start message.\n");
        recv(sock2, &start_mes, sizeof(start_mes), 0);
        printf("opponent name : %s\n", start_mes.opponent_username);
        simpleSnapshotPrinting(&start_mes.first_snapshot);
    }
    else { printf("error: unexpected message %d\n", message_type); }

    // game has started, player 0 makes a move
    printf("Player 0 makes a move.\n");
    MessageGameMove move_message;
    move_message.selected_house = 4;
    sendMessageGameMove(sock, move_message);

    recv(sock, &message_type, sizeof(int32_t), 0);
    if (message_type==GAME_UPDATE) {
        printf("Game update from server received.\n");
        MessageGameUpdate mes;
        recv(sock, &mes, sizeof(MessageGameUpdate), 0);
        simpleSnapshotPrinting(&mes.snapshot);
    }
    else { printf("error: unexpected message %d\n", message_type); }


    printf("Player 1 makes a move.\n");
    move_message.selected_house = 8;
    sendMessageGameMove(sock2, move_message);

    recv(sock, &message_type, sizeof(int32_t), 0);
    if (message_type==GAME_UPDATE) {
        printf("Game update from server received.\n");
        MessageGameUpdate mes;
        recv(sock, &mes, sizeof(MessageGameUpdate), 0);
        simpleSnapshotPrinting(&mes.snapshot);
    }
    else { printf("error: unexpected message %d\n", message_type); }

    // testing chat 
    // char message1[MAX_CHAT_MESSAGE_LENTGH] = "coucou";
    // sendMessageChat(sock3, message1); // message must be ignored by server because not in game
    // sleep(1);
    // sendMessageChat(sock2, message1); // message must be transmitted to user 0
    // recv(sock, &message_type, sizeof(int32_t), 0);
    // if (message_type == CHAT_MESSAGE) {
    //     char received_message[MAX_CHAT_MESSAGE_LENTGH];
    //     recv(sock, &received_message, MAX_CHAT_MESSAGE_LENTGH, 0);
    //     printf("user 0 received the message \"%s\" from opponent 1.\n", received_message);
    // }


    getchar();
    return 0;
}