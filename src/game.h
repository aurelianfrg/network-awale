

#define USERNAME_LENGTH 50

typedef struct House {
    unsigned int seeds;
} House;

enum Side {
    BOTTOM,
    TOP
};

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

typedef struct GameSnapshot {
    Board board; 
    Side turn;              // whose turn it is;
    unsigned int points[2]; 
} GameSnapshot; 

typedef struct Game {
    Player players[2];      // players[TOP] and players[BOTTOM]
    GameSnapshot snapshot;
} Game;




void initGame();


char isGameOver(GameSnapshot snapshot);
// tells whether the game is over by checking if any of the players has more than 12 points 

Side whoHasWon(GameSnapshot snapshot);
// return which player has won

