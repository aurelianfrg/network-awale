#pragma once 

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include "game.h"
#define MAX_CLIENTS 1024
#define MAX_CHAT_MESSAGE_LENTGH 1024


typedef enum MessageType {
    USER_CREATION,          // client -> server
    USER_REGISTRATION,      // server -> client
    GET_USER_LIST,          // client -> server
    SEND_USER_LIST,         // server -> client
    MATCH_REQUEST,          // client1 -> server
    MATCH_PROPOSITION,      // server -> client2
    MATCH_RESPONSE,         // client2 -> server | server -> client1
    MATCH_CANCELLATION,     // client1 -> server | server -> client
    GAME_START,             // server -> client1 & client2
    GAME_UPDATE,            // server -> client
    GAME_END,               // server -> client
    GAME_MOVE,              // client -> server
    GAME_ILLEGAL_MOVE,      // server -> client
    CHAT_MESSAGE            // client1 -> server | server -> client2
} MessageType;


typedef struct MessageUserCreation {
    char username[USERNAME_LENGTH];
} MessageUserCreation;

typedef struct MessageUserRegistration {
    int32_t user_id;
} MessageUserRegistration;

typedef struct MessageMatchRequest {
    int32_t opponent_id;
} MessageMatchRequest;

typedef struct MessageGameStart {
    char opponent_username[USERNAME_LENGTH];
    int32_t player_side;
    GameSnapshot first_snapshot;
} MessageGameStart;

typedef struct MessageGameUpdate {
    GameSnapshot snapshot;
} MessageGameUpdate;

typedef struct MessageGameEnd {
    int32_t winner;
    GameSnapshot final_snapshot;
} MessageGameEnd;

typedef struct MessageGameMove {
    int32_t selected_house;
} MessageGameMove;

typedef struct MessageMatchProposition {
    int32_t opponent_id;
    char opponent_username[USERNAME_LENGTH];
} MessageMatchProposition;

typedef struct MessageChat {
    char message[MAX_CHAT_MESSAGE_LENTGH];
    char username[USERNAME_LENGTH];
    int32_t user_id;
} MessageChat;




int isMessageComplete(int32_t message_type, ssize_t r);
// returns the difference between message expected lentgh and actual received lentgh
// if < 0, message was too long and most likely another message was transmitted at the same time
// if = 0, the message was in full
// if > 0, the message was only partly received

void sendMessageUserCreation(int fd, MessageUserCreation message);
void sendMessageUserRegistration(int fd, MessageUserRegistration message);
void sendMessageMatchRequest(int fd, MessageMatchRequest message);
void sendMessageGameStart(int fd, MessageGameStart message);
void sendMessageGameUpdate(int fd, MessageGameUpdate message);
void sendMessageGameEnd(int fd, MessageGameEnd message);
void sendMessageGameMove(int fd, MessageGameMove message);
void sendMessageGameIllegalMove(int fd);

void sendMessageGetUserList(int fd);
void sendUserList(int fd, char usernames[MAX_CLIENTS][USERNAME_LENGTH], int32_t user_ids[MAX_CLIENTS], int32_t usernames_count);

void sendMessageMatchResponse(int fd, int response);
void sendMessageMatchProposition(int fd, MessageMatchProposition message);
void sendMessageMatchCancellation(int fd);

void sendMessageChat(int fd, MessageChat message);
