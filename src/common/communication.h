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


enum MessageType {
    USER_CREATION,          // client -> server
    USER_REGISTRATION,      // server -> client
    GET_USER_LIST,
    SEND_USER_LIST,
    QUEUE_REQUEST,          // client -> server
    QUEUE_ACKNWOLEDGEMENT,  // server -> client
    GAME_START,             // server -> client
    GAME_UPDATE,            // server -> client
    GAME_END,               // server -> client
    GAME_MOVE,              // client -> server
    GAME_ILLEGAL_MOVE       // server -> client
};


typedef struct MessageUserCreation {
    char username[USERNAME_LENGTH];
} MessageUserCreation;

typedef struct MessageUserRegistration {
    unsigned int user_id;
} MessageUserRegistration;

typedef struct MessageQueueRequest {
    User user;
} MessageQueueRequest;

typedef struct MessageGameStart {
    User opponent;
    Side player_side;
    GameSnapshot first_snapshot;
} MessageGameStart;

typedef struct MessageGameUpdate {
    GameSnapshot snapshot;
} MessageGameUpdate;

typedef struct MessageGameEnd {
    User winner;
} MessageGameEnd;

typedef struct MessageGameMove {
    GameSnapshot snapshot;
} MessageGameMove;


int isMessageComplete(int message_type, ssize_t r);
// returns the difference between message expected lentgh and actual received lentgh
// if < 0, message was too long and most likely another message was transmitted at the same time
// if = 0, the message was in full
// if > 0, the message was only partly received

void sendMessageUserCreation(int fd, MessageUserCreation message);
void sendMessageUserRegistration(int fd, MessageUserRegistration message);
void sendMessageQueueRequest(int fd, MessageQueueRequest message);
void sendMessageGameStart(int fd, MessageGameStart message);
void sendMessageGameUpdate(int fd, MessageGameUpdate message);
void sendMessageGameEnd(int fd, MessageGameEnd message);
void sendMessageGameMove(int fd, MessageGameMove message);

void sendMessageGetUserList(int fd);
void sendUserList(int fd, char usernames[MAX_CLIENTS][USERNAME_LENGTH], int user_ids[MAX_CLIENTS], int usernames_count);