#pragma once 

#include "game.h"


enum MessageType {
    USER_CREATION,          // client -> server
    USER_REGISTRATION,      // server -> client
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

void sendMessageUserCreation(int fd, MessageUserCreation message);