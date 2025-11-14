#include "communication.h"


    // USER_CREATION,          // client -> server
    // USER_REGISTRATION,      // server -> client
    // QUEUE_REQUEST,          // client -> server
    // QUEUE_ACKNWOLEDGEMENT,  // server -> client
    // GAME_START,             // server -> client
    // GAME_UPDATE,            // server -> client
    // GAME_END,               // server -> client
    // GAME_MOVE,              // client -> server
    // GAME_ILLEGAL_MOVE       // server -> client


int isMessageComplete(int32_t message_type, ssize_t r) {
// returns the difference between message expected lentgh and actual received lentgh
// if < 0, message was too long and most likely another message was transmitted at the same time
// if = 0, the message was in full
// if > 0, the message was only partly received
    int expected;
    switch (message_type) {
        case USER_CREATION:
            expected = sizeof(int32_t) + sizeof(MessageUserCreation) ;
            break;
        case GET_USER_LIST:
            expected = sizeof(int32_t);
            break;
        case MATCH_REQUEST:
            expected = sizeof(int32_t) + sizeof(MessageMatchRequest);
            break;
        case GAME_MOVE:
            expected = sizeof(int32_t) + sizeof(MessageGameMove);
            break;       
        case MATCH_RESPONSE:
            expected = sizeof(int32_t) * 2;
            break;
        case MATCH_CANCELLATION:
            expected = sizeof(int32_t);
            break;
        case CHAT_MESSAGE:
            expected = sizeof(int32_t) + sizeof(MessageChat);
            break;
        default: 
            printf("error: unknown length for message of type %d.\n", message_type);
            exit(-1);
    }
    return expected - r;
}


void sendMessageUserCreation(int fd, MessageUserCreation message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageUserCreation message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = USER_CREATION;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}


void sendMessageUserRegistration(int fd, MessageUserRegistration message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageUserRegistration message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = USER_REGISTRATION;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}


void sendMessageMatchRequest(int fd, MessageMatchRequest message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageMatchRequest message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = MATCH_REQUEST;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}


void sendMessageGameStart(int fd, MessageGameStart message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageGameStart message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = GAME_START;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}

void sendMessageGameUpdate(int fd, MessageGameUpdate message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageGameUpdate message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = GAME_UPDATE;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}
void sendMessageGameEnd(int fd, MessageGameEnd message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageGameEnd message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = GAME_END;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}
void sendMessageGameMove(int fd, MessageGameMove message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageGameMove message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = GAME_MOVE;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}


// single signal messages 

void sendMessageQueueAcknowledgement(int fd) {

    int ack = USER_REGISTRATION;
    send(fd, &ack, sizeof(ack), 0);
}

void sendMessageIllegalMove(int fd) {

    int ack = GAME_ILLEGAL_MOVE;
    send(fd, &ack, sizeof(ack), 0);
}

void sendMessageGetUserList(int fd) {
    int32_t message_type = GET_USER_LIST;
    send(fd, &message_type, sizeof(message_type), 0);
}

void sendUserList(int fd, char usernames[MAX_CLIENTS][USERNAME_LENGTH], int32_t user_ids[MAX_CLIENTS], int32_t usernames_count, char in_game[MAX_CLIENTS]) {

    // send number of users, then usernames, then ids
    int32_t message_type = SEND_USER_LIST;

    ssize_t message_length = sizeof(message_type) * 2 + usernames_count*USERNAME_LENGTH*sizeof(char) + sizeof(usernames_count)*usernames_count;
    char* buffer = calloc(1, message_length);
    char* writing_adress = buffer;
    memcpy(writing_adress, &message_type, sizeof(message_type));
    writing_adress += sizeof(message_type);
    memcpy(writing_adress, &usernames_count, sizeof(usernames_count));
    writing_adress += sizeof(usernames_count);
    memcpy(writing_adress, usernames, usernames_count*USERNAME_LENGTH*sizeof(char));
    writing_adress += usernames_count*USERNAME_LENGTH*sizeof(char);
    memcpy(writing_adress, user_ids, sizeof(int32_t)*usernames_count);
    writing_adress += sizeof(int32_t)*usernames_count;

    send(fd, buffer, message_length, 0);
}

void sendMessageMatchResponse(int fd, int response) {
    typedef struct MessageWithHeader {
        int32_t message_type;
        int32_t response;
    } MessageWithHeader;
    MessageWithHeader message_with_header;

    message_with_header.message_type = MATCH_RESPONSE;
    message_with_header.response = response;
    send(fd, &message_with_header, sizeof(message_with_header),0);
}

void sendMessageMatchProposition(int fd, MessageMatchProposition message) {
    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageMatchProposition message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = MATCH_PROPOSITION;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}

void sendMessageMatchCancellation(int fd) {
    int32_t msg = MATCH_CANCELLATION;
    send(fd, &msg, sizeof(msg), 0);
}

void sendMessageGameIllegalMove(int fd) {
    int32_t msg = GAME_ILLEGAL_MOVE;
    send(fd, &msg, sizeof(msg), 0);
}

void sendMessageChat(int fd, MessageChat message) {

    typedef struct MessageWithHeader {
        int32_t message_type;
        MessageChat message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = CHAT_MESSAGE;
    message_with_header.message = message;
    send(fd, &message_with_header, sizeof(MessageWithHeader), 0);
}


// void sendMessageXXX(int fd, MessageXXX message) {

//     typedef struct MessageWithHeader {
//         int32_t message_type;
//         MessageXXX message;
//     } MessageWithHeader;

//     MessageWithHeader message_with_header;
//     message_with_header.message_type = USER_REGISTRATION;
//     message_with_header.message = message;

//     send(fd, &message_with_header, sizeof(message_with_header), 0);
// }
