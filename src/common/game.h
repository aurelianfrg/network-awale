#pragma once 


// constants 

#define USERNAME_LENGTH 50


// data structures 

typedef struct House {
    unsigned int seeds;
} House;

typedef enum Side {
    BOTTOM,
    TOP
} Side;

typedef struct User {
    unsigned int id;
    char username[USERNAME_LENGTH];
} User;

typedef struct Player {
    User user;
    Side side;              // 0 : bottom, 1 : top
} Player;

typedef struct Board {
    House houses[12];
} Board;
// houses are filled the following way in the array;
//  11 10 9  8  7  6
//  0  1  2  3  4  5  

typedef struct GameSnapshot {
    Board board; 
    Side turn;              // whose turn it is;
    unsigned int points[2]; 
} GameSnapshot; 

typedef struct Game {
    Player players[2];      // players[TOP] and players[BOTTOM]
    GameSnapshot snapshot;
} Game;


void simpleGamePrinting(Game* game);

Game* initGame(User user1, User user2);

int playMove(Game* game, int turn, int selected_house);
//play the action of a move 

void finishGame(Game* game);

char isGameOver(GameSnapshot snapshot);
// tells whether the game is over by checking if any of the players has more than 12 points 

Side whoHasWon(GameSnapshot snapshot);
// return which player has won

