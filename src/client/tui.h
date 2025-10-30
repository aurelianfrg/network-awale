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

typedef enum CellCharStyle {
    NO_STYLE,
    BOLD,
    FAINT,
    ITALIC,
    UNDERLINE,
    BLINKING,
    REVERSE,
    STRIKETROUGH
} CellCharStyle;

typedef struct CellChar {
    char cell[4];
    char size;
    char fgColorCode;
    char bgColorCode;
    CellCharStyle style;
} CellChar;

typedef struct GridCharBuffer {
    CellChar** buf;
    int byte_size;
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
void putGcbuf(GridCharBuffer* gcbuf, int row, int col, char* s, int bytes);
void flushFrame(GridCharBuffer* gcbuf);

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

// DRAW
void drawRows(GridCharBuffer* gcbuf);
void drawBox(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos); 
void drawBoxWithOffset(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos, int offset_row, int offset_col); 
void drawStrongBox(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos); 
void drawStrongBoxWithOffset(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos, int offset_row, int offset_col); 
void drawText(GridCharBuffer* gcbuf, char* txt, ScreenPos pos); 
void drawTextWithOffset(GridCharBuffer* gcbuf, char* txt, ScreenPos pos, int offset_row, int offset_col); 
void drawTitle(GridCharBuffer* gcbuf, ScreenPos pos); 
void drawTitleWithOffset(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col); 