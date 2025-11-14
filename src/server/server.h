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

int handleMessage(int32_t message_type, void* message_ptr, ssize_t r, User* users[MAX_CLIENTS], struct pollfd *pfds, int user_fd, int user_index);
void disconnectUser(int* user_index, int fd, User* users[MAX_CLIENTS], struct pollfd *pfds, int * nfds);
void cancel_game(Game* game);
void cancel_invite(Game* game);
void add_observer(User* observer, User* player_to_observe);
void remove_observer(User* observer);