
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game.h"

User* createUser(const char name[]) {
    static int id_count = 0;

    User* user = (User*) malloc(sizeof(User));
    strcpy(user->username, name);
    user->id = id_count++;

    return user;
}

Game* initGame(User * user1, User * user2) {

    Game* game = (Game*) malloc(sizeof(Game));

    // create players
    game->players[0] = user1;
    user1->side = TOP;
    game->players[1] = user2;
    user2->side = BOTTOM;
    
    // fill houses with 4 seeds
    for (int i = 0; i < 12; ++i) {
        game->snapshot.board.houses[i].seeds = 4;
    }
    game->snapshot.turn = BOTTOM;
    game->snapshot.points[0] = 0;
    game->snapshot.points[1] = 0;

    return game;
}

int next_house(int house) {
    return (house+1)%12;
}

int precedent_house(int house) {
    return (house-1)%12;
}

int playMove(Game* game, Side turn, int selected_house) {

    // check inputs
    if (turn != TOP && turn != BOTTOM) {
        return -1;
    }
    if (turn == TOP && selected_house < 6) {
        return -2;
    }
    if (turn == BOTTOM && selected_house > 5) {
        return -3;
    }
    
    if (game->snapshot.board.houses[selected_house].seeds == 0) {
        return -4;
    }

    // play the move : dispatch the seeds
    int seeds_to_dispatch = game->snapshot.board.houses[selected_house].seeds;
    int house_to_fill = selected_house;
    while (seeds_to_dispatch > 0) {
        house_to_fill = next_house(house_to_fill);
        if (house_to_fill == selected_house) continue;
        ++(game->snapshot.board.houses[house_to_fill].seeds);
        --seeds_to_dispatch;
    }
    game->snapshot.board.houses[selected_house].seeds = 0;

    // check if there were seeds captured
    bool capturableSeeds = true;
    int house_to_check = house_to_fill;
    while (capturableSeeds) {
        int seeds = game->snapshot.board.houses[house_to_check].seeds;
        if (seeds == 2 || seeds == 3) {
            game->snapshot.points[turn] += game->snapshot.board.houses[house_to_check].seeds;
            game->snapshot.board.houses[house_to_fill].seeds = 0;
        }
        else {
            capturableSeeds = false;
        }
        house_to_check = precedent_house(house_to_check);
    }
    
    // turn is over, change game turn
    game->snapshot.turn = !(game->snapshot.turn);
    
    return 0;
}

void simpleGamePrinting(Game* game) {
    printf("%s (%d - BOTTOM) vs %s (%d - TOP) : %s to play\n\n", 
        game->players[0]->username, 
        game->players[0]->id, 
        game->players[1]->username, 
        game->players[1]->id, 
        (game->snapshot.turn == BOTTOM ? game->players[BOTTOM]->username : game->players[TOP]->username) 
    );

    // print top side
    for (int i = 11; i > 5; --i) {
        printf(" %d ", game->snapshot.board.houses[i].seeds);
    }

    printf("\n");

    // print bottom side
    for (int i = 0; i < 6; ++i) {
        printf(" %d ", game->snapshot.board.houses[i].seeds);
    }
    printf("\n\n");
}
