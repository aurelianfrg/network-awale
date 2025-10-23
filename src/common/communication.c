#include "communication.h"
#include <sys/socket.h>

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
