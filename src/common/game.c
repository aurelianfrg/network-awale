

#include "game.h"
#include <stdlib.h>


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
