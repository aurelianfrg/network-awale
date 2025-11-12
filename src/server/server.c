/* server.c
 *
 * Simple multi-client echo server using poll().
 *
 * Build: gcc -o server server.c
 * Run:   ./server 12345
 *
 * Author: example
 */

#include "server.h"

// GAME LOGIC


// CONNECTION LOGIC
static volatile sig_atomic_t keep_running = 1;

void int_handler(int _) { (void)_; keep_running = 0; }

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void disconnectUser(int* user_index, int fd, User* users[MAX_CLIENTS], struct pollfd *pfds, int * nfds) {

    if (users[*user_index] == NULL) {
        // user already disconnected
        return;
    }

    printf("Client %d with fd %d disconnected.\n", *user_index, fd);
    close(fd);

    // remove this pfds entry by shifting
    pfds[*user_index] = pfds[*nfds - 1];
    pfds[*nfds - 1].fd = -1;
    pfds[*nfds - 1].events = 0;
    pfds[*nfds - 1].revents = 0;    

    // deallocate user if it exists
    if (users[*user_index] != NULL) {
        // end active game it there is one
        if (users[*user_index]->active_game != NULL && users[*user_index]->active_game->cancelled_game == false) {
            users[*user_index]->active_game->cancelled_game = true;
            if (users[*user_index]->active_game->players[BOTTOM] != NULL) {
                sendMessageMatchCancellation(users[*user_index]->active_game->players[BOTTOM]->fd);
                users[*user_index]->active_game->players[BOTTOM]->active_game = NULL;
            }
            if (users[*user_index]->active_game != NULL && users[*user_index]->active_game->players[TOP] != NULL) {
                sendMessageMatchCancellation(users[*user_index]->active_game->players[TOP]->fd);
                users[*user_index]->active_game->players[TOP] = NULL;
            }
            
            free(users[*user_index]->active_game);
        }

        free(users[*user_index]);
        users[*user_index] = NULL;
    }

    users[*user_index] = users[*nfds-1];
    users[*nfds-1] = NULL;
    --(*nfds);
    --(*user_index); // check the moved one on next iteration
}



int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, int_handler);
    signal(SIGTERM, int_handler);

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    User* users[MAX_CLIENTS];
    memset(users, 0, sizeof(User*)*MAX_CLIENTS);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (set_nonblocking(listen_fd) < 0) {
        perror("set_nonblocking");
        // not fatal; continue
    }

    struct pollfd *pfds = calloc(MAX_CLIENTS + 1, sizeof(struct pollfd));
    if (!pfds) {
        perror("calloc");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    pfds[0].fd = listen_fd;
    pfds[0].events = POLLIN;
    int nfds = 1; // number of used entries in pfds

    printf("Server listening on port %d\n", port);

    char buf[BUF_SIZE];

    while (keep_running) {
        int timeout_ms = 1000; // wakeup every second to check signal
        int ready = poll(pfds, nfds, timeout_ms);
        if (ready < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        } else if (ready == 0) {
            continue; // timeout, loop again to check keep_running
        }

        // Check listening socket
        if (pfds[0].revents & POLLIN) {
            // accept in a loop (because non-blocking)
            while (1) {
                struct sockaddr_in cli_addr;
                socklen_t cli_len = sizeof(cli_addr);
                int client_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
                if (client_fd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    perror("accept");
                    break;
                }

                // find free slot
                if (nfds >= MAX_CLIENTS + 1) {
                    fprintf(stderr, "Too many connections, rejecting\n");
                    close(client_fd);
                    continue;
                }

                if (set_nonblocking(client_fd) < 0) {
                    // not fatal
                }

                pfds[nfds].fd = client_fd;
                pfds[nfds].events = POLLIN;
                nfds++;

                char ipbuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cli_addr.sin_addr, ipbuf, sizeof(ipbuf));
                printf("\nAccepted %s:%d (fd=%d)\n", ipbuf, ntohs(cli_addr.sin_port), client_fd);
            }
        }

        // Check client sockets (start at 1)
        for (int i = 1; i < nfds; ++i) {
            int fd = pfds[i].fd;
            short re = pfds[i].revents;
            if (re == 0) continue;

            if (re & (POLLERR | POLLHUP | POLLNVAL)) {
                // client disconnected/error
                disconnectUser(&i, fd, users, pfds, &nfds);             
                continue;
            }

            if (re & POLLIN) {
                ssize_t r = recv(fd, buf, sizeof(buf), 0);
                if (r < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                    perror("recv");
                    disconnectUser(&i, fd, users, pfds, &nfds);                
                    continue;

                } else if (r == 0) {
                    // client closed
                    printf("Client %d with fd %d disconnected.\n", i, fd);
                    disconnectUser(&i, fd, users, pfds, &nfds);               
                    continue;

                } else {
                    // standard case : process how you handle the client when a message was received
                    printf("\nMessage length : %lu\n",r);
                    int message_type;
                    memcpy(&message_type, buf, sizeof(int));
                    void* message_ptr = (void*) ((int*)buf+1);

                    // TODO: change this mecanism to handle the case where several message are received at once in the same buffer
                    int success = handleMessage(message_type, message_ptr, r, users, pfds, fd, i);

                    if (success < 0) {
                        printf("Something went wrong handling message from user with file descriptor %d\n", fd);
                    }
                }
            }
        }
    }

    printf("Shutting down server...\n");
    // Close all open fds
    for (int i = 1; i < nfds; ++i) {
        close(pfds[i].fd);
        if (users[i] != NULL) free(users[i]);
    }
    free(pfds);
    close(listen_fd);

    return EXIT_SUCCESS;
}


int handleMessage(int message_type, void* message_ptr, ssize_t r, User* users[MAX_CLIENTS], struct pollfd *pfds, int user_fd, int user_index) {

    // checking if message was received in full
    int diff = isMessageComplete(message_type, r);
    if (diff != 0) {
        printf("lentgh difference between expected and received message size : %d\n", diff);
        if (diff > 0) return -1; //message not received in full -> cancel operation        
    }

    User* source_user = users[user_index];

    switch (message_type) {

        case USER_CREATION:
            if (source_user != NULL){
                printf("error: User is already registered.\n");
                return -1;
            }
            MessageUserCreation userCreationMes;
            memcpy(&userCreationMes, message_ptr, sizeof(MessageUserCreation));

            printf("User creation message received\n");
            printf("username : %s\n", userCreationMes.username);

            User* instanciated_user = createUser(userCreationMes.username, user_fd);
            // update user 
            users[user_index] = instanciated_user;

            //acknowledge client 
            MessageUserRegistration msg;
            msg.user_id = instanciated_user->id;
            sendMessageUserRegistration(user_fd, msg);

            break;

        case GET_USER_LIST:
            if (source_user == NULL) {
                printf("error: Got a request from an unregistered user.\n");
                return -1;
            }
            printf("Received users list request from %s.\n", users[user_index]->username);
            fflush(stdout);

            // list all users 
            char usernames[MAX_CLIENTS][USERNAME_LENGTH];
            int user_ids[MAX_CLIENTS];
            int users_count = 0;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (users[i] != NULL && i != user_index) {
                    // we found an user 
                    strcpy(usernames[users_count], users[i]->username);
                    user_ids[users_count] = users[i]->id;
                    ++users_count;
                }
            }

            // send result to client
            sendUserList(user_fd, usernames, user_ids, users_count);

            break;

        case MATCH_REQUEST:
            // check that user is indeed created
            if (source_user == NULL) {
                printf("error: Got a request from an unregistered user.\n");
                return -1;
            }
            printf("Received a match request from user %d.\n", user_index);

            // read message
            MessageMatchRequest mes;
            memcpy(&mes, message_ptr, sizeof(MessageMatchRequest));

            // find opponent user by id
            User* opponent = NULL;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (users[i] != NULL && users[i]->id == mes.opponent_id) {
                    opponent = users[i];
                }
            }
            if (opponent == NULL) {
                printf("Unable to find opponent with asked id.\n");
                sendMessageMatchResponse(user_fd, false);
                return -1;
            }

            // check that opponent exists 
            if (opponent == source_user) {
                printf("error: cannot create a game with oneself.\n");
                sendMessageMatchResponse(user_fd, false); // auto cancellation if non-existing or self opponent
            }
            else if (source_user->active_game != NULL || opponent->active_game != NULL) {
                printf("error: an user already has an active game.\n");
                sendMessageMatchResponse(user_fd, false); // one of 2 users is already in a game
            }
            else if (source_user->pending_game != NULL || opponent->pending_game != NULL) {
                printf("error: an user already has a pending invite.\n");
                sendMessageMatchResponse(user_fd, false); // one of 2 users already has an invite
            }
            else {
                printf("Received game request from user %s (id %d) with user %s (id %d).\n", source_user->username, source_user->id, opponent->username, opponent->id);

                Game * new_game = calloc(1, sizeof(Game));
                new_game->players[BOTTOM] = source_user; 
                new_game->players[TOP] = opponent;
                source_user->pending_game = new_game;       
                opponent->pending_game = new_game;

                // send invite
                MessageMatchProposition invite_msg;
                invite_msg.opponent_id = source_user->id;
                strcpy(invite_msg.opponent_username, source_user->username);
                
                int target_fd = opponent->fd;
                sendMessageMatchProposition(target_fd, invite_msg);
            }

            break;

        case MATCH_RESPONSE:
            // check that user is indeed created
            if (source_user == NULL) {
                printf("error: Got a request from an unregistered user.\n");
                return -1;
            }
            
            // check it has indeed a pending game
            if (source_user->pending_game == NULL) {
                printf("error: user %d (%s) sent a response but has no game invite pending.\n", user_index, source_user->username);
                return -1;
            }

            // check it has no active game
            if (source_user->active_game != NULL) {
                printf("error: user %d (%s) sent a response but has an ongoing game.\n", user_index, source_user->username);
                return -1;
            }

            // read msg 
            int32_t response;
            memcpy(&response, message_ptr, sizeof(response));

            if (response == true && source_user->pending_game->cancelled_game == false && source_user->pending_game->players[TOP] == source_user) {
                // start the game
                source_user->active_game = source_user->pending_game;
                source_user->active_game->players[BOTTOM]->active_game = source_user->active_game;
                source_user->active_game->accepted_game = true;
                setupGame(source_user->active_game);

                source_user->active_game->players[BOTTOM]->pending_game = NULL;
                source_user->pending_game = NULL;

                printf("Done instanciating a game : \n");
                simpleGamePrinting(source_user->active_game);
            }
            else {
                return -1;
            }

            break;

        case MATCH_CANCELLATION:
            // check that user is indeed created
            if (source_user == NULL) {
                printf("error: Got a request from an unregistered user.\n");
                return -1;
            }
            
            // check it has indeed a pending game or an active game
            if (source_user->pending_game == NULL && source_user->active_game == NULL) {
                printf("error: user %d (%s) sent a cancellation but has no game invite pending or active game.\n", user_index, source_user->username);
                return -1;
            }
            if (source_user->pending_game != NULL && source_user->active_game != NULL) {
                printf("error: user %d (%s) has both active and pending game.\n", user_index, source_user->username);
                return -1;
            }

            // apply cancellation
            if (source_user->active_game != NULL) {
                printf("request from user %d (%s) to cancel active game.\n", user_index, source_user->username);
                // cancel active game
                source_user->active_game->cancelled_game = true;
                sendMessageMatchCancellation(source_user->active_game->players[TOP]->fd);
                sendMessageMatchCancellation(source_user->active_game->players[BOTTOM]->fd);
                Game* active_game = source_user->active_game;
                active_game->players[BOTTOM]->active_game = NULL;
                active_game->players[TOP]->active_game = NULL;
            }
            else {
                printf("request from user %d (%s) to cancel pending game invite.\n", user_index, source_user->username);
                source_user->pending_game->cancelled_game = true;
                sendMessageMatchCancellation(source_user->pending_game->players[TOP]->fd);
                sendMessageMatchCancellation(source_user->pending_game->players[BOTTOM]->fd);
                Game* pending_game = source_user->pending_game;
                pending_game->players[BOTTOM]->pending_game = NULL;
                pending_game->players[TOP]->pending_game = NULL;
            }
            break;

        case GAME_MOVE:
            // check that user is indeed created
            if (source_user == NULL) {
                printf("error: Got a request from an unregistered user.\n");
                return -1;
            }
            
            // check it has indeed an active game
            if (source_user->active_game == NULL) {
                printf("error: user %d (%s) played a move but has no active game.\n", user_index, source_user->username);
                return -1;
            }

            MessageGameMove move_message;
            memcpy(&move_message, message_ptr, sizeof(MessageGameMove));
            Game* game = source_user->active_game;
            
            // check it was the right user that played the move
            int valid_move;
            if ( game->snapshot.turn == BOTTOM && source_user == game->players[BOTTOM]) {
                printf("BOTTOM user %d (%s) played the move %d", user_index, source_user->username, move_message.selected_house);
                valid_move = (playMove(game, BOTTOM, move_message.selected_house) == 0);
            }
            if ( game->snapshot.turn == TOP && source_user == game->players[TOP]) {
                printf("TOP user %d (%s) played the move %d", user_index, source_user->username, move_message.selected_house);
                valid_move = (playMove(game, TOP, move_message.selected_house) == 0);
            }
            else {
                valid_move = false;
            }

            if (!valid_move) {
                sendMessageGameIllegalMove(source_user->fd);
            }
            else {
                printf("Valide move played by user %d (%s), game updated.\n", user_index, source_user->username);
                MessageGameUpdate update;
                update.snapshot = game->snapshot;
                sendMessageGameUpdate(game->players[BOTTOM]->fd, update);
                sendMessageGameUpdate(game->players[TOP]->fd, update);
                simpleGamePrinting(game);
            }

        default:
            return -1;
        

    }
    return 0;
}

