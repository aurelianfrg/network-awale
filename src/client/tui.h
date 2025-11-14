#pragma once

// Norme POSIX.1-2008 (SUSv4)
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

// ===================================
// ======== RAW TERMINAL STUFF =======
// ===================================

typedef enum TerminalKeyCode {
    KEY_ESCAPE = 27,
    KEY_BACKSPACE = 127,
    KEY_ENTER = 13,
    KEY_ARROW_LEFT = 1000000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_DEL,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
} TerminalKeyCode;

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
    int cursor_x;
    int cursor_y;
    int cursor_visibility;
    TerminalConfig term_config;
} TuiState;

typedef struct TextStyle {
    char flags;         // 8 bits -> les 8 flags de style
    char fg_color_code; // 8 bits -> code couleur https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b#256-colors
    char bg_color_code; // 8 bits -> code couleur https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b#256-colors
} TextStyle;

typedef enum TextStyleFlags {
    // 8 flags max
    FG_COLOR, // Si marqué, la couleur sera prise de fg_color_code
    BG_COLOR, // Si marqué, la couleur sera prise de bg_color_code
    BOLD,
    FAINT,
    ITALIC, // des fois, italique s'affiche comme inverse
    UNDERLINE,
    STRIKETHROUGH,
    INVERSE,
} TextStyleFlags;

typedef struct CellChar {
    // On veut garder une cellule assez petite en mémoire -> - de 64 bits
    unsigned char uchar[4];      // 32 bits -> pour n'importe quel caractère unicode
    TextStyle style;    // 24 bits -> amplement suffisant
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

typedef struct u_string {
    char* buf;
    int byte_len;
    int char_len;
} u_string;

extern TextStyle* NO_STYLE;

void initApplication();

// GRIDCHARBUFFER
GridCharBuffer* createGcbuf(int rows, int cols);
void freeGcbuf(GridCharBuffer* gcbuf);
void putGcbuf(GridCharBuffer* gcbuf, int row, int col, char* s, int bytes, TextStyle* style);
void flushFrame(GridCharBuffer* gcbuf);
int getFlagState(char source, char flag_pos);
void setFlagState(char* source, char flag_pos, char state);
char addStyleFlag(char style_flags, TextStyleFlags flag);
char mkStyleFlags(int flag_count, ...);
int getTextStyleSize(TextStyle* text_style);
int getCellCharSize(CellChar* cell_char);
int getGcbufSize(GridCharBuffer* gcbuf);

// INPUT
void processKeypress(int key);
void editorMoveCursor(int key);

// OUTPUT
void setCursorPos(int row, int col);
void setCursorPosRelative(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col);
void hideCursor();
void showCursor();
void terminalClearScreen();
void drawFrame();
void drawFrameDebug();
void frameContent(GridCharBuffer* gcbuf);
void getDrawPosition(int* offset_row, int* offset_col, ScreenPos pos, GridCharBuffer* gcbuf, int width, int height);

// strlen for unicode strings
size_t u_strlen(char *s);
// length for one unicode char
size_t u_charlen(unsigned char *s);
int isAlpha(int c);
int isNumeric(int c);
int isAlphaNumeric(int c);
int isAnyAsciiChar(int c);
int isAnyValidChar(int c);
int u_codeToBytes(unsigned int c, unsigned char* out);
int u_bytesToCode(unsigned char* in);
int u_strAppend(u_string* s, int codepoint);
int u_strPop(u_string* s);

// DRAW
void drawDebugColors(GridCharBuffer* gcbuf);
void drawSolidRect(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, int width, int height, TextStyle* style);
void drawBox(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int width, int height); 
void drawStrongBox(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int width, int height); 
void drawText(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, char* text); 
void drawTextWithRawStyle(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, char* text, TextStyle* style); 
void drawButton(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, char* text, unsigned char color_code, int selected); 
void drawTitle(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col); 
void drawPopup(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int width, int height, char* text); 