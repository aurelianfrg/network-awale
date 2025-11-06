#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/game.h"

int main() {
    User* user1 = createUser("Anatou");
    User* user2 = createUser("Aurel");

    Game * game = initGame(*user1, *user2);
    simpleGamePrinting(game);

    int valid_move;
    
    valid_move = playMove(game, BOTTOM, 2);
    printf("valid move ? %d\n", valid_move);
    simpleGamePrinting(game);

    valid_move = playMove(game, BOTTOM, 2);
    printf("valid move ? %d\n", valid_move);
    simpleGamePrinting(game);

    return 0;
}