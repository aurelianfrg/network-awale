#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>
#include <fcntl.h>

#include "../common/communication.h"

#define BACKLOG 16
#define BUF_SIZE 4096

int handle_message(int message_type, void* message_ptr, User* users[MAX_CLIENTS], int user_fd, int user_index);