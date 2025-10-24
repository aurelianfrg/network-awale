#pragma once

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

typedef struct editorConfig {
    int cx, cy;
    int screenrows;
    int screencols;
    struct termios orig_termios;
} editorConfig;

// Append buffer -> stores the next frame before flushing
typedef struct abuf {
    char *b;
    int len;
} abuf;

typedef enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
} editorKey;

// APPEND BUFFER
void abAppend(abuf *ab, const char *s, int len);
void abFree(abuf *ab);

// TERMINAL
void die(const char *s);
void initEditor();
void disableRawMode();
void enableRawMode();
int editorReadKey();
int getWindowSize(int* rows, int* cols);
int getCursorPosition(int *rows, int *cols);

// INPUT
void editorProcessKeypress();
void editorMoveCursor(int key);

// OUTPUT
void editorClearScreen();
void editorRefreshScreen();
void editorDrawRows();