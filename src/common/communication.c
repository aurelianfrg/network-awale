#include "communication.h"
#include <sys/socket.h>

    // USER_CREATION,          // client -> server
    // USER_REGISTRATION,      // server -> client
    // QUEUE_REQUEST,          // client -> server
    // QUEUE_ACKNWOLEDGEMENT,  // server -> client
    // GAME_START,             // server -> client
    // GAME_UPDATE,            // server -> client
    // GAME_END,               // server -> client
    // GAME_MOVE,              // client -> server
    // GAME_ILLEGAL_MOVE       // server -> client


void sendMessageUserCreation(int fd, MessageUserCreation message) {

    typedef struct MessageWithHeader {
        int message_type;
        MessageUserCreation message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = USER_CREATION;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}


void sendMessageUserRegistration(int fd, MessageUserRegistration message) {

    typedef struct MessageWithHeader {
        int message_type;
        MessageUserRegistration message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = USER_REGISTRATION;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}


void sendMessageQueueRequest(int fd, MessageQueueRequest message) {

    typedef struct MessageWithHeader {
        int message_type;
        MessageQueueRequest message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = QUEUE_REQUEST;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}


void sendMessageGameStart(int fd, MessageGameStart message) {

    typedef struct MessageWithHeader {
        int message_type;
        MessageGameStart message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = GAME_START;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}

void sendMessageGameUpdate(int fd, MessageGameUpdate message) {

    typedef struct MessageWithHeader {
        int message_type;
        MessageGameUpdate message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = GAME_UPDATE;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}
void sendMessageGameEnd(int fd, MessageGameEnd message) {

    typedef struct MessageWithHeader {
        int message_type;
        MessageGameEnd message;
    } MessageWithHeader;

    MessageWithHeader message_with_header;
    message_with_header.message_type = GAME_END;
    message_with_header.message = message;

    send(fd, &message_with_header, sizeof(message_with_header), 0);
}
void sendMessageGameMove(int fd, MessageGameMove message) {

    typedef struct MessageWithHeader {
        int message_type;
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

// void sendMessageXXX(int fd, MessageXXX message) {

//     typedef struct MessageWithHeader {
//         int message_type;
//         MessageXXX message;
//     } MessageWithHeader;

//     MessageWithHeader message_with_header;
//     message_with_header.message_type = USER_REGISTRATION;
//     message_with_header.message = message;

//     send(fd, &message_with_header, sizeof(message_with_header), 0);
// }
