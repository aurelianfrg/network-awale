
#include <stdlib.h>
#include <stdio.h>

#include "game.h"


Game* initGame(User user1, User user2) {

    Game* game = (Game*) malloc(sizeof(Game));

    // create players
    game->players[0].user = user1;
    game->players[0].side = TOP;
    game->players[1].user = user2;
    game->players[1].side = BOTTOM;
    
    // fill houses with 4 seeds
    for (int i = 0; i < 12; ++i) {
        game->snapshot.board.houses[i].seeds = 4;
    }
    game->snapshot.turn = BOTTOM;
    game->snapshot.points[0] = 0;
    game->snapshot.points[1] = 0;

    return game;
}

int playMove(Game* game, int turn, int selected_house) {

    // check inputs
    if (turn != TOP || turn != BOTTOM) {
        return -1;
    }
    if (turn == TOP && selected_house < 6) {
        return -1;
    }
    if (turn == BOTTOM && selected_house > 5) {
        return -1;
    }
    
    if (game->snapshot.board.houses[selected_house].seeds == 0) {
        return -1;
    }

    // TODO: dev the functionality
}

void simpleGamePrinting(Game* game) {
    printf("%s (TOP) vs %s (BOTTOM)\n\n", game->players[0].user.username, game->players[1].user.username);

    // print top side
    for (int i = 11; i > 5; --i) {
        printf(" %d ", game->snapshot.board.houses[i].seeds);
    }

    printf("\n");

    // print bottom side
    for (int i = 0; i < 6; ++i) {
        printf(" %d ", game->snapshot.board.houses[i].seeds);
    }
    printf("\n");
}
