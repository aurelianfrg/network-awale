#pragma once 


// constants 

#define USERNAME_LENGTH 100 // or 25 4-bytes UTF-8 chars
#define true 1
#define false 0
#define bool char

// data structures 
typedef struct Game Game;

typedef struct House {
    unsigned int seeds;
} House;

typedef enum Side {
    BOTTOM,
    TOP
} Side;

typedef struct User {
    int fd;
    char username[USERNAME_LENGTH];
    Game* active_game;
    Game* pending_game;     // placeholder for when an invitation is received
} User;

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
    bool accepted_game;
    bool cancelled_game;
    User* players[2];      // players[TOP] and players[BOTTOM]
    GameSnapshot snapshot;
} Game;


// --- Game functionalities --- 

void simpleGamePrinting(Game* game);
// simply print the state of a game

Game* initGame(User* user1, User* user2);
// init a game between users

void setupGame(Game* game);

User* createUser(const char name[], int fd) ;
// create an user with a name and a unique id

int playMove(Game* game, Side turn, int selected_house);
// play the action of a move and updates the game snapshot
// returns a negative value if the move was not allowed

void finishGame(Game* game);

char isGameOver(GameSnapshot snapshot);
// tells whether the game is over by checking if any of the players has more than 12 points 

Side whoHasWon(GameSnapshot snapshot);
// return which player has won

