#include "client.h"

NavigationState navigationState = USER_CREATION_MENU;

// NETWORKING
int sock;
struct sockaddr_in srv;

// DISPLAY
#define GENERAL_DISPLAY_BUF_MAX_LENGTH 512
char general_display_buf[GENERAL_DISPLAY_BUF_MAX_LENGTH];
char is_waiting = 0;
char is_waiting_for_game_response = 0;
char is_game_request_pending = 0;
char game_request_selected_field = 0;

#define ACTION_COLOR 2
#define ACCEPT_COLOR 2
#define REFUSE_COLOR 13
#define BACK_COLOR 13
#define QUIT_COLOR 17
// GAME
char user_pseudo_buf[USERNAME_LENGTH];
u_string user_pseudo = { user_pseudo_buf, 0, 0 };

User connected_user;
User opponent_user;

Board current_board;

int users_list_count = 0;
char users_list_buf[MAX_CLIENTS][USERNAME_LENGTH];
int32_t users_list_id[MAX_CLIENTS];

// BUTTONS
int selected_field = 0;
int field_count = 1;
#define PLAY_BUTTON 0
#define BACK_BUTTON 1
#define QUIT_BUTTON 2

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
        drawButton(gcbuf, CENTER, 2, 0, "Jouer", ACTION_COLOR, selected_field==PLAY_BUTTON);
        drawButton(gcbuf, CENTER, 8, 0, "Retour", BACK_COLOR, selected_field==BACK_BUTTON);
        drawButton(gcbuf, CENTER, 10, 0, "Quitter", QUIT_COLOR, selected_field==QUIT_BUTTON);
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

    case GAME_START_MENU:
        hideCursor();
        drawTitle(gcbuf, TOP_CENTER, 8, 0);
        break;
    
    case IN_GAME_MENU:
        hideCursor();
        drawTitle(gcbuf, TOP_CENTER, 8, 0);
        drawAwaleBoard(gcbuf, CENTER, 0, 0);
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
        char game_request_msg[150]; sprintf(game_request_msg, "%s - %d", opponent_user.username, opponent_user.id);
        drawPopup(gcbuf, CENTER, 0, 0, NO_STYLE, 45, 3, "");
        drawText(gcbuf, CENTER, -1, 0, "Vous avez été invité à jouer contre");
        drawText(gcbuf, CENTER, 0, 0, game_request_msg);
        drawButton(gcbuf, CENTER, 1, -10, "Accepter", ACCEPT_COLOR, game_request_selected_field==0);
        drawButton(gcbuf, CENTER, 1, 11, "Refuser", REFUSE_COLOR, game_request_selected_field==1);
    }

    if (is_waiting) {
        drawPopup(gcbuf, CENTER, 0, 0, NO_STYLE, 25, 5, "WAITING...");
    }
}

void changeMenu(NavigationState new_menu) {
    navigationState = new_menu;
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
        if (!is_waiting && !is_waiting_for_game_response) {
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
                if (is_game_request_pending) handle_game_request_popup(c);
                else if (c==KEY_ARROW_UP && selected_field>0) selected_field--;
                else if (c==KEY_ARROW_DOWN && selected_field<field_count-1) selected_field++;
                else if (c==KEY_ENTER) switch (selected_field)
                    {
                    case PLAY_BUTTON:
                        sendMessageGetUserList(sock);
                        is_waiting = 1;
                        // Call will block UI, upon server response, the player list will be shown
                        break;
                    case BACK_BUTTON:
                        changeMenu(USER_CREATION_MENU);
                        break;
                    case QUIT_BUTTON:
                        terminalClearScreen();
                        exit(0);
                        break;
                    
                    default:
                        die("Selected field is invalid");
                        break;
                    }
                break;

            case USER_LIST_MENU:
                if (is_game_request_pending) handle_game_request_popup(c);
                else if (c==KEY_ARROW_UP && selected_field>0) selected_field--;
                else if (c==KEY_ARROW_DOWN && selected_field<field_count-1) selected_field++;
                else if (c==KEY_BACKSPACE) changeMenu(MAIN_MENU);
                else if (c==KEY_ENTER) {
                    MessageMatchRequest mes = { users_list_id[selected_field] };
                    sendMessageMatchRequest(sock, mes);
                    changeMenu(GAME_START_MENU);
                }
                break;
            
            case IN_GAME_MENU:
                if (c==KEY_ARROW_LEFT && selected_awale_house>0) selected_awale_house--;
                else if (c==KEY_ARROW_RIGHT && selected_awale_house<5) selected_awale_house++;
                break;
            
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
            is_waiting = 0;
            changeMenu(MAIN_MENU);
            break;
        case MATCH_RESPONSE:
            int32_t res;
            r = recieve_from_server(&res, sizeof(int32_t));
            if (res)
                changeMenu(IN_GAME_MENU);
            else
                changeMenu(MAIN_MENU);
            is_waiting = 0;
            break;
        case GAME_START:
            changeMenu(IN_GAME_MENU);
            dieNoError("OKKKK");
            
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

void handle_game_request_popup(int c) {
    if (c==KEY_ARROW_LEFT) game_request_selected_field = 0;
    else if (c==KEY_ARROW_RIGHT) game_request_selected_field = 1;
    else if (c==KEY_ENTER) {
        sendMessageMatchResponse(sock, !game_request_selected_field);
        is_game_request_pending = 0;
        if (!game_request_selected_field)
            changeMenu(GAME_START_MENU);
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
    // printf("Connected to %s:%d. Type messages and press Enter.\n", server_ip, port);
}

#define AWALE_HOUSE_WIDTH 9
#define AWALE_HOUSE_HEIGHT 4

void drawAwaleHouse(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int seed_count, int side) {
    int width = AWALE_HOUSE_WIDTH;
    int height = AWALE_HOUSE_HEIGHT;

    drawBox(gcbuf, TOP_LEFT, offset_row, offset_col, style, width-2, height-2);
    char buf[20]; sprintf(buf, "%d", seed_count);
    if (side == TOP)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row, offset_col+width-2-strlen(buf), buf, style);
    else
        drawTextWithRawStyle(gcbuf, TOP_LEFT, offset_row+height-1, offset_col+width-2-strlen(buf), buf, style);

    int drawn_seeds = 0;
    drawSolidRect(gcbuf, TOP_LEFT, offset_row+1, offset_col+1, height-1, width-1, style);
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

void drawAwaleBoard(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col) {
    int board_width = AWALE_HOUSE_WIDTH*6 + 5;
    int board_height = AWALE_HOUSE_HEIGHT*2 + 1;

    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, board_width, board_height);
    pos_row += offset_row;
    pos_col += offset_col;

    TextStyle ennemy_style = { mkStyleFlags(1, FAINT), 0, 0 };
    TextStyle player_style = { 0, 0, 0 };
    TextStyle player_style_selected = { player_style.flags | mkStyleFlags(1, INVERSE), player_style.fg_color_code, player_style.bg_color_code };
    TextStyle center_style = { mkStyleFlags(1, FAINT), 0, 0 };
    for (int i=0; i<6; i++)
        drawAwaleHouse(gcbuf, TOP_LEFT, pos_row+offset_row, pos_col+(AWALE_HOUSE_WIDTH+1)*i+offset_col, &ennemy_style, 4, TOP);
    for (int i=0; i<6; i++) {
        if (selected_awale_house == i)
            drawAwaleHouse(gcbuf, TOP_LEFT, pos_row+board_height-AWALE_HOUSE_HEIGHT+offset_row, pos_col+(AWALE_HOUSE_WIDTH+1)*i+offset_col, &player_style_selected, 4, BOTTOM);
        else
            drawAwaleHouse(gcbuf, TOP_LEFT, pos_row+board_height-AWALE_HOUSE_HEIGHT+offset_row, pos_col+(AWALE_HOUSE_WIDTH+1)*i+offset_col, &player_style, 4, BOTTOM);
    }
    for (int i=0; i<board_width; i++)
        drawTextWithRawStyle(gcbuf, TOP_LEFT, pos_row+board_height/2+offset_row, pos_col+i, "═", &center_style);
}