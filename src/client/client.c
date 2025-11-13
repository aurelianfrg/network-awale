#include "client.h"

NavigationState navigationState = USER_CREATION_MENU;

// NETWORKING
int sock;
struct sockaddr_in srv;

// DISPLAY
#define GENERAL_DISPLAY_BUF_MAX_LENGTH 512
char general_display_buf[GENERAL_DISPLAY_BUF_MAX_LENGTH];

#define ACTION_COLOR 2
#define ACCEPT_COLOR 2
#define REFUSE_COLOR 13
#define BACK_COLOR 13
#define QUIT_COLOR 17

// POPUPS
char is_waiting = 0;
char is_notified = 0;
char notification_message[200];
char is_waiting_for_game_response = 0;
char is_game_request_pending = 0;

// USER CREATION
char user_pseudo_buf[USERNAME_LENGTH];
u_string user_pseudo = { user_pseudo_buf, 0, 0 };

// GAME
Side connected_user_side = 0;
User connected_user;
User opponent_user;
Side opponent_user_side = 0;
Side winning_side;

// USER LIST
GameSnapshot current_game_snapshot;

int users_list_count = 0;
char users_list_buf[MAX_CLIENTS][USERNAME_LENGTH];
int32_t users_list_id[MAX_CLIENTS];

// BUTTONS
char game_request_selected_field = 0; // Game request popup
int selected_field = 0;
int field_count = 1;
#define MM_PLAY_BUTTON 0
#define MM_BACK_BUTTON 1
#define MM_QUIT_BUTTON 2

#define IG_AWALE_HOUSE 0
#define IG_BACK_BUTTON 1

int selected_awale_house = 0;

//char server_mes[100] = "Waiting for server message...\0";

int display_color_test=0;

int main(int argc, char **argv) {
    if (argc != 3) dieNoError("Usage: COMMAND <server-ip> <port>");

    int port = atoi(argv[2]);
    const char *server_ip = argv[1];
    connect_to_server(server_ip, port, &sock, &srv);

    initApplication();

    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[1].fd = sock;
    pfds[1].events = POLLIN;

    while (1) {
        drawFrame();
        processEvents(pfds);
    }

    close(sock);
    return EXIT_SUCCESS;
}

void frameContent(GridCharBuffer* gcbuf) {
    char conn_infos[50];
    snprintf(conn_infos, 50, "Connecté à !{ub}%s:%d", inet_ntoa(srv.sin_addr), srv.sin_port);
    drawText(gcbuf, BOTTOM_RIGHT, -1, 0, conn_infos);
    drawText(gcbuf, BOTTOM_LEFT, -1, 0, "!{ub}Ctrl-C!{r} pour quitter");
    //drawText(gcbuf, BOTTOM_CENTER, -1, 0, server_mes);

    switch (navigationState)
    {
    case USER_CREATION_MENU:
        drawTitle(gcbuf, CENTER, -5, 0);
        drawText(gcbuf,CENTER, 5, 0, "Bienvenue");
        drawText(gcbuf,CENTER, 7, 0, "Votre pseudonyme !{if}(<ENTRÉE> pour valider)!{r}:");
        drawBox(gcbuf, CENTER, 9, 0, NO_STYLE, USERNAME_LENGTH/4+2, 1);
        drawText(gcbuf,CENTER, 9, 0, user_pseudo.buf);
        setCursorPosRelative(gcbuf, CENTER, 9, (user_pseudo.char_len+1)/2);
        break;

    case MAIN_MENU:
        hideCursor();
        drawTitle(gcbuf, TOP_CENTER, 8, 0);

        drawButton(gcbuf, CENTER, 2, 0, "Jouer", 2, selected_field==MM_PLAY_BUTTON);
        drawButton(gcbuf, CENTER, 8, 0, "Retour", 13, selected_field==MM_BACK_BUTTON);
        drawButton(gcbuf, CENTER, 10, 0, "Quitter", 17, selected_field==MM_QUIT_BUTTON);
        sprintf(general_display_buf, "Pseudo: !{bi}%s - %d!{r}", connected_user.username, connected_user.id);
        drawText(gcbuf, BOTTOM_CENTER, -2, 0, general_display_buf);
        break;

    case USER_LIST_MENU:
        hideCursor();
        drawPopup(gcbuf, CENTER, 0, 0, NO_STYLE, gcbuf->cols-10, gcbuf->rows-4, "");
        char title[100]; sprintf(title, "!{u}%d joueurs connectés!{r}", users_list_count+1);
        char user[150]; sprintf(user, "!{bi}%s - %d (Vous)!{r}", connected_user.username, connected_user.id);
        int i = 0;
        int row = 4;
        int col = 7;
        drawText(gcbuf, BOTTOM_CENTER, -3, 0, "!{u}Retour Arr.!{r}: Retour | !{u}Entrée!{r}: Inviter");
        drawText(gcbuf, TOP_CENTER, 2, 0, title);
        drawText(gcbuf, TOP_LEFT, row, col, user); row++;
        TextStyle selected_style = { mkStyleFlags(1, INVERSE), 0, 0 };
        while (i<users_list_count) {
            sprintf(user, "%s - %d", users_list_buf[i], users_list_id[i]);
            drawTextWithRawStyle(gcbuf, TOP_LEFT, row, col, user, (i==selected_field)?&selected_style:NO_STYLE);
            row++;
            if (row > gcbuf->rows-3) {
                row = 3;
                col += 30;
            }
            i++;
        }
        break;
    
    case IN_GAME_MENU:
        hideCursor();
        TextStyle top_style = { mkStyleFlags(1, FAINT), 0, 0 };
        TextStyle bot_style = { 0, 0, 0 };
        if (current_game_snapshot.turn==connected_user_side)
            drawText(gcbuf, TOP_CENTER, 1, 0, "!{vF004}Votre tour");
        else 
            drawText(gcbuf, TOP_CENTER, 1, 0, "!{vF004}Tour de l'adversaire");

        drawAwaleBoard(gcbuf, CENTER, 0, 0, &top_style, &bot_style);
        sprintf(general_display_buf, "Opposant: !{u}%s-%d!{r}", opponent_user.username, opponent_user.id);
        drawText(gcbuf, CENTER, -8, 0, general_display_buf);
        sprintf(general_display_buf, "%d points", current_game_snapshot.points[opponent_user_side]);
        drawText(gcbuf, CENTER, -7, 0, general_display_buf);
        sprintf(general_display_buf, "Vous: !{u}%s-%d!{r}", connected_user.username, connected_user.id);
        drawText(gcbuf, CENTER, 7, 0, general_display_buf);
        sprintf(general_display_buf, "%d points", current_game_snapshot.points[connected_user_side]);
        drawText(gcbuf, CENTER, 8, 0, general_display_buf);
        drawButton(gcbuf, BOTTOM_CENTER, -2, 0, "Retour", 13, selected_field==IG_BACK_BUTTON);
        break;

    case GAME_END_MENU:
        hideCursor();
        TextStyle faint_style = { mkStyleFlags(1, FAINT), 0, 0 };
        User* winner = (winning_side==connected_user_side)?&connected_user:&opponent_user; 
        sprintf(general_display_buf, "Victoire de !{u}%s-%d!{r}", winner->username, winner->id);
        drawPopup(gcbuf, CENTER, -10, 0, NO_STYLE, u_strlen(general_display_buf), 1, general_display_buf);
        // Points
        sprintf(general_display_buf, "Opposant: !{u}%s-%d!{r}", opponent_user.username, opponent_user.id);
        drawText(gcbuf, CENTER, -6, 10, general_display_buf);
        sprintf(general_display_buf, "%d points", current_game_snapshot.points[opponent_user_side]);
        drawText(gcbuf, CENTER, -5, 10, general_display_buf);
        sprintf(general_display_buf, "Vous: !{u}%s-%d!{r}", connected_user.username, connected_user.id);
        drawText(gcbuf, CENTER, -6, -10, general_display_buf);
        sprintf(general_display_buf, "%d points", current_game_snapshot.points[connected_user_side]);
        drawText(gcbuf, CENTER, -5, -10, general_display_buf);
        drawAwaleBoard(gcbuf, CENTER, 4, 0, &faint_style, &faint_style);
        drawButton(gcbuf, BOTTOM_CENTER, -2, 0, "Retourner à l'accueil", 13, 1);

        break;
    
    default:
        break;
    }

    if (display_color_test) {
        drawSolidRect(gcbuf, TOP_LEFT, 0, 0, gcbuf->rows, gcbuf->cols, NO_STYLE);
        drawDebugColors(gcbuf);
        setCursorPosRelative(gcbuf, BOTTOM_RIGHT, 0, 0);
    }

    if (is_game_request_pending) {
        sprintf(general_display_buf, "%s - %d", opponent_user.username, opponent_user.id);
        drawPopup(gcbuf, CENTER, 0, 0, NO_STYLE, 45, 3, "");
        drawText(gcbuf, CENTER, -1, 0, "Vous avez été invité à jouer contre");
        drawText(gcbuf, CENTER, 0, 0, general_display_buf);
        drawButton(gcbuf, CENTER, 1, -10, "Accepter", ACCEPT_COLOR, game_request_selected_field==0);
        drawButton(gcbuf, CENTER, 1, 11, "Refuser", REFUSE_COLOR, game_request_selected_field==1);
    }

    if (is_waiting_for_game_response) {
        sprintf(general_display_buf, "%s - %d", opponent_user.username, opponent_user.id);
        drawPopup(gcbuf, CENTER, 0, 0, NO_STYLE, 45, 3, "");
        drawText(gcbuf, CENTER, -1, 0, "En attente d'une réponse de");
        drawText(gcbuf, CENTER, 0, 0, general_display_buf);
        drawButton(gcbuf, CENTER, 1, 0, "Annuler", REFUSE_COLOR, 1);
    }

    if (is_notified) {
        drawPopup(gcbuf, CENTER, 0, 0, NO_STYLE, u_strlen(notification_message)+4, 2, notification_message);
        drawButton(gcbuf, CENTER, 0, 0, "OK", BACK_COLOR, 1);
    }

    if (is_waiting) {
        drawPopup(gcbuf, CENTER, 0, 0, NO_STYLE, 25, 5, "WAITING...");
    }
}

void changeMenu(NavigationState new_menu) {
    navigationState = new_menu;
    // Entering state
    switch (new_menu) {
    case USER_CREATION_MENU:
        user_pseudo.buf[0] = '\0';
        user_pseudo.byte_len = 0;
        user_pseudo.char_len = 0;
        break;

    case MAIN_MENU:
        selected_field = 0;
        field_count = 3;
        break;

    case USER_LIST_MENU:
        selected_field = 0;
        field_count = users_list_count;
        break;

    case IN_GAME_MENU:
        selected_field = 0;
        field_count = 2;
        break;
    default:
        break;
    }
}


void processEvents(struct pollfd pfds[2]) {
    // Poll all events from STDIN and SOCK
    int sock = pfds[1].fd;
    int ready = poll(pfds, 2, -1);
    if (ready < 0) {
        if (errno == EINTR) return;
        close(sock);
        die("poll");
    }

    // stdin
    if (pfds[0].revents & POLLIN) {
        int c = terminalReadKey();
        processKeypress(c);
        // GENERAL KEYBINDS
        switch (c) {
        case CTRL_KEY('c'):
            terminalClearScreen();
            exit(0);
            break;
        case CTRL_KEY('t'):
            display_color_test = ~display_color_test;
            break;
        }
        if (is_notified) {
            handle_notification(c);
        }
        else if (!is_waiting) {
            // CONTEXTUAL KEYBINDS
            switch (navigationState) {
            case USER_CREATION_MENU:
                // Curly braces are not allowed in pseudo (for display reasons)
                if (isAnyValidChar(c) && c!=123 && c!=125 && user_pseudo.char_len<USERNAME_LENGTH/4) 
                    u_strAppend(&user_pseudo, c);
                else if (c==KEY_BACKSPACE) u_strPop(&user_pseudo);
                else if (c==KEY_ENTER) {
                    MessageUserCreation mes;
                    strcpy(mes.username,user_pseudo.buf);
                    sendMessageUserCreation(sock, mes);
                    is_waiting = 1;
                    changeMenu(MAIN_MENU);
                }
                break;

            case MAIN_MENU:
                if (is_waiting_for_game_response) handle_waiting_for_game_response(c);
                else if (is_game_request_pending) handle_game_request_popup(c);
                else if (c==KEY_ARROW_UP && selected_field>0) selected_field--;
                else if (c==KEY_ARROW_DOWN && selected_field<field_count-1) selected_field++;
                else if (c==KEY_ENTER) switch (selected_field)
                    {
                    case MM_PLAY_BUTTON:
                        sendMessageGetUserList(sock);
                        is_waiting = 1;
                        break;
                    case MM_BACK_BUTTON:
                        changeMenu(USER_CREATION_MENU);
                        break;
                    case MM_QUIT_BUTTON:
                        terminalClearScreen();
                        exit(0);
                        break;
                    
                    default:
                        die("Main menu: Selected field is invalid");
                        break;
                    }
                    break;

            case USER_LIST_MENU:
                if (is_waiting_for_game_response) handle_waiting_for_game_response(c);
                else if (is_game_request_pending) handle_game_request_popup(c);
                else if (c==KEY_ARROW_UP && selected_field>0) selected_field--;
                else if (c==KEY_ARROW_DOWN && selected_field<field_count-1) selected_field++;
                else if (c==KEY_BACKSPACE) changeMenu(MAIN_MENU);
                else if (c==KEY_ENTER) {
                    MessageMatchRequest mes = { users_list_id[selected_field] };
                    sendMessageMatchRequest(sock, mes);
                    opponent_user.id = users_list_id[selected_field];
                    strcpy(opponent_user.username, users_list_buf[selected_field]);
                    is_waiting_for_game_response = 1;
                }
                break;
            
            case IN_GAME_MENU:
                if (c==KEY_ARROW_LEFT && selected_field==IG_AWALE_HOUSE && selected_awale_house>0) selected_awale_house--;
                else if (c==KEY_ARROW_RIGHT && selected_field==IG_AWALE_HOUSE && selected_awale_house<5) selected_awale_house++;
                else if (c==KEY_ARROW_UP && selected_field>0) selected_field--;
                else if (c==KEY_ARROW_DOWN && selected_field<field_count-1) selected_field++;
                else if (c==KEY_ENTER && selected_field==IG_BACK_BUTTON) {
                    sendMessageMatchCancellation(sock);
                    changeMenu(MAIN_MENU);
                }
                else if (c==KEY_ENTER && selected_field==IG_AWALE_HOUSE) {
                    MessageGameMove mes = {
                        (connected_user_side==BOTTOM)?
                        selected_awale_house:
                        selected_awale_house+6
                    };
                    sendMessageGameMove(sock, mes);
                    current_game_snapshot.turn = !current_game_snapshot.turn;
                }
                break;

            case GAME_END_MENU:
                if (c==KEY_ENTER) changeMenu(MAIN_MENU);
            
            default:
                break;
            }
        }
    }

    // socket
    if (pfds[1].revents & POLLIN) {
        int32_t message_type; ssize_t r;
        r = recieve_from_server(&message_type, sizeof(int32_t));
    
        switch (message_type) {
        case USER_REGISTRATION:
            r = recieve_from_server(&(connected_user.id), sizeof(int32_t));
            strcpy(connected_user.username, user_pseudo.buf);
            is_waiting = 0;
            changeMenu(MAIN_MENU);
            break;

        case SEND_USER_LIST:
            int32_t user_count;
            r = recieve_from_server(&user_count, sizeof(int32_t));
            users_list_count = user_count;
            if (user_count > 0) {
                r = recieve_from_server(users_list_buf, sizeof(char)*user_count*USERNAME_LENGTH);
                r = recieve_from_server(users_list_id, sizeof(int32_t)*user_count);
            }
            is_waiting = 0;
            changeMenu(USER_LIST_MENU);
            break;

        case MATCH_PROPOSITION:
            r = recieve_from_server(&(opponent_user.id), sizeof(int32_t));
            r = recieve_from_server(&(opponent_user.username), sizeof(char)*USERNAME_LENGTH);
            is_game_request_pending = 1;
            game_request_selected_field = 0;
            break;

        case MATCH_CANCELLATION:
            is_game_request_pending = 0;
            is_waiting_for_game_response = 0;
            is_waiting = 0;
            if (navigationState == IN_GAME_MENU || is_waiting_for_game_response) {
                is_notified = 1;
                strcpy(notification_message, "Partie annulée");
            }
            changeMenu(MAIN_MENU);
            break;

        case MATCH_RESPONSE:
            int32_t res;
            r = recieve_from_server(&res, sizeof(int32_t));
            if (res)
                changeMenu(IN_GAME_MENU);
            else
                changeMenu(MAIN_MENU);
                is_notified = 1;
                strcpy(notification_message, "Partie refusée");
            is_waiting = 0;
            is_waiting_for_game_response = 0;
            break;

        case GAME_START:
            changeMenu(IN_GAME_MENU);
            is_waiting = 0;
            is_waiting_for_game_response = 0;
            r = recieve_from_server(&(opponent_user.username), sizeof(char)*USERNAME_LENGTH);
            r = recieve_from_server(&connected_user_side, sizeof(int32_t));
            opponent_user_side = !connected_user_side;
            r = recieve_from_server(&current_game_snapshot, sizeof(GameSnapshot));
            break;

        case GAME_UPDATE:
            r = recieve_from_server(&current_game_snapshot, sizeof(GameSnapshot));
            break;

        case GAME_END:
            r = recieve_from_server(&winning_side, sizeof(int32_t));
            r = recieve_from_server(&current_game_snapshot, sizeof(GameSnapshot));
            changeMenu(GAME_END_MENU);
            break;

        case GAME_ILLEGAL_MOVE:
            strcpy(notification_message, "This move is illegal");
            is_notified = 1;
            break;

        default:
            char buf[100]; sprintf(buf, "UNKWOWN MESSAGE: %d", message_type);
            dieNoError(buf);
            break;
        }
    }

    if (pfds[1].revents & (POLLHUP | POLLERR | POLLNVAL)) {
        die("Socket closed or error.");
    }
}

void handle_notification(int c) {
    if (c==KEY_ENTER) {
        is_notified = 0;
    }
}

void handle_waiting_for_game_response(int c) {
    if (c==KEY_ENTER) {
        sendMessageMatchCancellation(sock);
        is_waiting_for_game_response = 0;
    }
}

void handle_game_request_popup(int c) {
    if (c==KEY_ARROW_LEFT) game_request_selected_field = 0; // Accept
    else if (c==KEY_ARROW_RIGHT) game_request_selected_field = 1; // Refuse
    else if (c==KEY_ENTER) {
        sendMessageMatchResponse(sock, !game_request_selected_field);
        is_game_request_pending = 0;
        // Si on a accepté la game
        if (!game_request_selected_field)
            is_waiting = 1;
    }
}

ssize_t recieve_from_server(void* buffer, size_t size) {
    ssize_t r = recv(sock, buffer, size, 0);
    if (r < 0) {
        close(sock);
        die("recv");
    } else if (r == 0) {
        close(sock);
        die("Server closed connection.\n");
    } else {
        return r;
    }
}

void connect_to_server(const char* server_ip, int port, int* sock_out, struct sockaddr_in* srv_out) {
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %d\n", port);
        die("Invalid port");
    }

    *sock_out = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_out < 0) die("socket"); 

    memset(srv_out, 0, sizeof(*srv_out));
    srv_out->sin_family = AF_INET;
    srv_out->sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, server_ip, &(srv_out->sin_addr)) != 1) {
        fprintf(stderr, "inet_pton failed for %s\n", server_ip);
        close(*sock_out);
        die("inet_pton failed");
    }

    if (connect(*sock_out, (struct sockaddr *)srv_out, sizeof(*srv_out)) < 0) {
        close(*sock_out);
        die("connect");
    }
}

#define AWALE_HOUSE_WIDTH 9
#define AWALE_HOUSE_HEIGHT 4

void drawAwaleHouse(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int seed_count, int side) {
    int width = AWALE_HOUSE_WIDTH;
    int height = AWALE_HOUSE_HEIGHT;

    drawSolidRect(gcbuf, TOP_LEFT, offset_row+1, offset_col+1, width-2, height-2, style);
    drawBox(gcbuf, TOP_LEFT, offset_row, offset_col, style, width-2, height-2);
    char buf[20]; sprintf(buf, "%d", seed_count);
    if (side == TOP)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row, offset_col+width-2-strlen(buf), buf, style);
    else
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+height-1, offset_col+width-2-strlen(buf), buf, style);

    int drawn_seeds = 0;
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+1, offset_col+1+i, ".", style);
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+2, offset_col+1+i, ".", style);
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+2, offset_col+1+i, ":", style);
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+1, offset_col+1+i, ":", style);
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++) 
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+2, offset_col+1+i, ((i%2)==0)?"∴":"∵", style);
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+1, offset_col+1+i, ((i%2)==0)?"∴":"∵", style);
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+2, offset_col+1+i, "∷", style);
    for (int i=0; i<width-2 && drawn_seeds<seed_count; i++, drawn_seeds++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+1, offset_col+1+i, "∷", style);

}

void drawAwaleBoard(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* top_style, TextStyle* bot_style) {
    int board_width = AWALE_HOUSE_WIDTH*6 + 5;
    int board_height = AWALE_HOUSE_HEIGHT*2 + 1;

    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, board_width, board_height);
    pos_row += offset_row;
    pos_col += offset_col;

    TextStyle bot_style_selected = { bot_style->flags | mkStyleFlags(1, INVERSE), bot_style->fg_color_code, bot_style->bg_color_code };
    TextStyle center_style = { mkStyleFlags(1, FAINT), 0, 0 };
    TextStyle* drawnStyle;
    // TOP HOUSES
    for (int i=0; i<6; i++) {
        unsigned int seedCount = (opponent_user_side==TOP)?
            current_game_snapshot.board.houses[11-i].seeds:
            current_game_snapshot.board.houses[5-i].seeds;
        drawAwaleHouse(
            gcbuf, TOP_LEFT, 
            pos_row, pos_col+(AWALE_HOUSE_WIDTH+1)*i, top_style, 
            seedCount, TOP
        );
    }
    // BOTTOM HOUSES
    for (int i=0; i<6; i++) {
        if (current_game_snapshot.turn != connected_user_side) drawnStyle = top_style;
        else if (selected_awale_house == i && selected_field==IG_AWALE_HOUSE) drawnStyle = &bot_style_selected;
        else drawnStyle = bot_style;
        unsigned int seedCount = (connected_user_side==BOTTOM)?
            current_game_snapshot.board.houses[i].seeds:
            current_game_snapshot.board.houses[i+6].seeds;
        drawAwaleHouse(
            gcbuf, TOP_LEFT, 
            pos_row+board_height-AWALE_HOUSE_HEIGHT, pos_col+(AWALE_HOUSE_WIDTH+1)*i, drawnStyle,
            seedCount, BOTTOM
        );
    }
    for (int i=0; i<board_width; i++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, pos_row+board_height/2+offset_row, pos_col+i, "═", &center_style);
}