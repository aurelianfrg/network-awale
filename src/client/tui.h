#pragma once

#include <stdio.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h> // nécessite -D_POSIX_SOURCE

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
TerminalConfig* initTerminal();
void closeTerminal();
void handleResize(int sig);

// TERMINAL INPUT
void disableRawMode();
void enableRawMode();
int editorReadKey();

// TERMINAL DISPLAY
void terminalClearScreen();

// TERMINAL INFOS
int getCursorPosition(int *rows, int *cols);
int getWindowSize(int* rows, int* cols);

// ===================================
// ============ TUI LOGIC ============
// ===================================

typedef struct ApplicationState {
    int cursorx;
    int cursory;
    TerminalConfig term_config;
} ApplicationState;

typedef struct CellChar {
    char cell[4];
    char size;
} CellChar;

typedef struct GridCharBuffer {
    CellChar** buf;
    int byte_size;
    int cols;
    int rows;
} GridCharBuffer;

// GRIDCHARBUFFER
GridCharBuffer* createGcbuf(int rows, int cols);
void freeGcbuf(GridCharBuffer* gcbuf);
void putGcbuf(GridCharBuffer* gcbuf, int row, int col, char* s, int bytes);
void flushFrame(GridCharBuffer* gcbuf);

// INPUT
void processKeypress();
void editorMoveCursor(int key);

// OUTPUT
void setCursorPos(int row, int col);
void terminalClearScreen();
void drawFrame();
void editorDrawRows(GridCharBuffer* gcbuf);
void drawBox(GridCharBuffer* gcbuf, int width, int height); 
void updateCursorPos(GridCharBuffer* gcbuf);