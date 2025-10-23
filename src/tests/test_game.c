#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/game.h"

int main() {
    User user1;
    User user2;
    strcpy(user1.username, "Anatou");
    strcpy(user2.username, "Aurel");

    Game * game = initGame(user1, user2);
    simpleGamePrinting(game);

    return 0;
}