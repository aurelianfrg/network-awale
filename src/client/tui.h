#pragma once

// Norme POSIX.1-2008 (SUSv4)
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

// ===================================
// ======== RAW TERMINAL STUFF =======
// ===================================

typedef enum terminalKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
} terminalKey;

typedef struct TerminalConfig {
    int screenrows;
    int screencols;
    struct termios orig_termios;
} TerminalConfig;

#define CTRL_KEY(k) ((k) & 0x1f)

// TERMINAL LIFECYCLE
void die(const char *s);
void dieNoError(const char *s);
TerminalConfig* initTerminal();
void closeTerminal();
void handleResize(int sig);

// TERMINAL INPUT
void disableRawMode();
void enableRawMode();
int terminalReadKey();

// TERMINAL DISPLAY
void terminalClearScreen();

// TERMINAL INFOS
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int* rows, int* cols);

// ===================================
// ============ TUI LOGIC ============
// ===================================

typedef struct TuiState {
    int cursorx;
    int cursory;
    TerminalConfig term_config;
} TuiState;

typedef enum CellCharStyleFlags {
    // 8 flags max
    FG_COLOR, // Si marqué, la couleur sera prise de fg_color_code
    BG_COLOR, // Si marqué, la couleur sera prise de bg_color_code
    BOLD,
    FAINT,
    ITALIC, // des fois, italique s'affiche comme inverse
    UNDERLINE,
    STRIKETHROUGH,
    INVERSE,
} CellCharStyleFlags;

// Les couleurs
typedef struct CellChar {
    // On veut garder une cellule assez petite en mémoire -> 64 bits
    char chr[4];       // 32 bits -> pour n'importe quel caractère unicode
    char style;         // 8 bits -> les 8 flags de style
    char fg_color_code;   // 8 bits -> code couleur https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b#256-colors
    char bg_color_code;   // 8 bits -> code couleur https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b#256-colors
} CellChar;

typedef struct GridCharBuffer {
    CellChar** buf;
    int cols;
    int rows;
} GridCharBuffer;

typedef enum ScreenPos {
    CENTER,
    TOP_LEFT,
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
} ScreenPos;

void initApplication();

// GRIDCHARBUFFER
GridCharBuffer* createGcbuf(int rows, int cols);
void freeGcbuf(GridCharBuffer* gcbuf);
void putGcbuf(GridCharBuffer* gcbuf, int row, int col, char* s, int bytes, char style, char fg_color_code, char bg_color_code);
void flushFrame(GridCharBuffer* gcbuf);
int getFlagState(char source, char flag_pos);
void setFlagState(char* source, char flag_pos, char state);
char mkStyleFlags(const CellCharStyleFlags flags[], int flag_n);
int getCellCharSize(CellChar* cell_char);
int getGcbufSize(GridCharBuffer* gcbuf);

// INPUT
void processKeypress();
void editorMoveCursor(int key);

// OUTPUT
void setCursorPos(int row, int col);
void setCursorPosRelative(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col);
void terminalClearScreen();
void drawFrame();
void drawFrameDebug();
void frameContent(GridCharBuffer* gcbuf);
void getDrawPosition(int* offset_row, int* offset_col, ScreenPos pos, GridCharBuffer* gcbuf, int width, int height);

size_t u_strlen(char *s);
size_t u_charlen(char *s);
// DRAW
void drawDebugColors(GridCharBuffer* gcbuf);
void drawSolidRect(GridCharBuffer* gcbuf, int start_row, int start_col, int end_row, int end_col, char color_code);
void drawBox(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos); 
void drawBoxWithOffset(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos, int offset_row, int offset_col); 
void drawStrongBox(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos); 
void drawStrongBoxWithOffset(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos, int offset_row, int offset_col); 
void drawText(GridCharBuffer* gcbuf, char* txt, ScreenPos pos); 
void drawTextWithOffset(GridCharBuffer* gcbuf, char* txt, ScreenPos pos, int offset_row, int offset_col); 
void drawTitle(GridCharBuffer* gcbuf, ScreenPos pos); 
void drawTitleWithOffset(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col); 