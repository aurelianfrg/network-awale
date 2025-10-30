#include "tui.h"

// ===================================
// ======== RAW TERMINAL STUFF =======
// ===================================

volatile sig_atomic_t resize_pending = 0;
TerminalConfig term_config;

void handleResize(int sig) {
    resize_pending = 1;
}

void die(const char *s) {
    terminalClearScreen();
    perror(s);
    write(STDOUT_FILENO, "\r\n", 2);
    exit(1);
}
void dieNoError(const char *s) {
    terminalClearScreen();
    write(STDOUT_FILENO, s, strlen(s));
    write(STDOUT_FILENO, "\r\n", 2);
    exit(1);
}

TerminalConfig* initTerminal() {
    enableRawMode();
    if (getWindowSize(&term_config.screenrows, &term_config.screencols) == -1) die("getWindowSize");
    return &term_config;
}

void closeTerminal() {
    disableRawMode();
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_config.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &term_config.orig_termios) == -1) die("tcgetattr");
    atexit(closeTerminal);
    struct termios raw = term_config.orig_termios;
    // Let's disable some terminal features
    // INPUT FLAGS
    // - ICRNL: controls Ctrl-M to \n translation
    // - IXON: controls Ctrl-V to type chars manually
    // OUTPUT FLAGS
    // - OPOST: controls output post-processing
    // LOCAL FLAGS (MISC)
    // - ECHO: controls displaying input char to terminal
    // - ICANON: controls Enter to send keys vs immediate mode
    // - ISIG: controls Ctrl-C and Ctrl-V
    // - IEXTEN: controls Ctrl-S and Ctrl-Q (data stop and resume)

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | IEXTEN | ICANON | ISIG);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int terminalReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1) {
            if (errno == EAGAIN) continue;
            if (errno == EINTR) continue;
            die("read");
        }
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

void terminalClearScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // Clear screen
    write(STDOUT_FILENO, "\x1b[H", 3); // Move cursor to top-left
}

int getCursorPosition(int *rows, int *cols) {
    // Some escape sequence manipulation to parse cursor position
    char buf[32];
    unsigned int i = 0;
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    // First we try to use ioctl, if it works, go to the 'else' clause, if it fails, use escape codes
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) { 
            die("getWindowSize"); 
        }
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

// ===================================
// ============ TUI LOGIC ============
// ===================================

TuiState app_state;

void initApplication() {
    app_state.cursorx = 0;
    app_state.cursory = 0;
    app_state.term_config = *initTerminal();
    // signal(SIGWINCH, handleResize);

    struct sigaction sa;
    sa.sa_handler = handleResize;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

// ========= INPUT ==========

void processKeypress() {
    int c = terminalReadKey();
    printf("c=%d\r\n", c);
    switch (c) {
        case CTRL_KEY('q'):
            terminalClearScreen();
            exit(0);
            break;
        case ARROW_UP:
        case ARROW_LEFT:
        case ARROW_DOWN:
        case ARROW_RIGHT:
        case 'z':
        case 'q':
        case 's':
        case 'd':
            editorMoveCursor(c);
            break;
        case PAGE_UP:
        case PAGE_DOWN:
            int times = app_state.term_config.screenrows;
            while (times--)
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            break;
        case HOME_KEY:
            app_state.cursorx = 0;
            break;
        case END_KEY:
            app_state.cursorx = app_state.term_config.screencols - 1;
            break;
    }
}

void editorMoveCursor(int key) {
  switch (key) {
    case ARROW_LEFT:
    case 'q':
        if (app_state.cursorx != 0) {
            app_state.cursorx--;
        }
        break;
    case ARROW_RIGHT:
    case 'd':
        if (app_state.cursorx != app_state.term_config.screencols - 1) {
            app_state.cursorx++;
        }
        break;
    case ARROW_UP:
    case 'z':
        if (app_state.cursory != 0) {
            app_state.cursory--;
        }
        break;
    case ARROW_DOWN:
    case 's':
        if (app_state.cursory != app_state.term_config.screenrows - 1) {
            app_state.cursory++;
        }
        break;
    }
}

// ========= OUTPUT ==========

void drawFrame() {
    if (resize_pending) {
        resize_pending = 0;
        if (getWindowSize(&app_state.term_config.screenrows, &app_state.term_config.screencols) == -1) die("getWindowSize");
    }
    sigset_t x,old;
    sigemptyset(&x);
    sigaddset(&x, SIGWINCH);
    int ret = sigprocmask(SIG_BLOCK, &x, &old);
    printf("is SIGWINCH in mask ? %d\n", sigismember(&old, SIGWINCH));
    printf("ret=%d\n", ret);

    // Prepare frame
    GridCharBuffer* gcbuf = createGcbuf(app_state.term_config.screenrows, app_state.term_config.screencols);

    // Draw frame
    frameContent(gcbuf);

    // flush frame
    flushFrame(gcbuf);
    freeGcbuf(gcbuf);
    sigprocmask(SIG_UNBLOCK, &x, NULL);
}

int frame_counter = 0;
void drawFrameDebug() {
    printf("====== FRAME %d ====== \r\n", frame_counter);
    frame_counter++;
    printf("resize_pending=%d\r\n", resize_pending);
    if (resize_pending) {
        resize_pending = 0;
        if (getWindowSize(&app_state.term_config.screenrows, &app_state.term_config.screencols) == -1) die("getWindowSize");
    }
    sigset_t x,old;
    sigemptyset(&x);
    sigaddset(&x, SIGWINCH);
    sigprocmask(SIG_SETMASK, NULL, &old);
    printf("is SIGWINCH in mask ? %d\r\n", sigismember(&old, SIGWINCH));
    printf("is SIGWINCH in x ? %d\r\n", sigismember(&x, SIGWINCH));
}

GridCharBuffer* createGcbuf(int rows, int cols) {
    GridCharBuffer* gcbuf = (GridCharBuffer*)malloc(sizeof(GridCharBuffer));
    CellChar** buf = (CellChar**)malloc(sizeof(CellChar*)*rows);
    gcbuf->buf = buf;
    gcbuf->cols = cols;
    gcbuf->rows = rows;
    gcbuf->byte_size = 0;
    for (int row=0; row<rows; row++) {
        buf[row] = (CellChar*)malloc(sizeof(CellChar)*cols);
        for (int col=0; col<cols; col++) {
            gcbuf->buf[row][col].size = 0;
            putGcbuf(gcbuf, row, col, " ", 1);
        }
    }
    return gcbuf;
}

void freeGcbuf(GridCharBuffer* gcbuf) {
    for (int row=0; row<gcbuf->rows; row++) free(gcbuf->buf[row]);
    free(gcbuf->buf);
    free(gcbuf);
}

void putGcbuf(GridCharBuffer* gcbuf, int row, int col, char* s, int bytes) {
    if (row > gcbuf->rows-1 || col > gcbuf->cols-1) return;
    if (row < 0 || col < 0) return;

    gcbuf->byte_size += bytes - gcbuf->buf[row][col].size;
    gcbuf->buf[row][col].size = bytes;
    for (int j=0; j<bytes; j++)
        gcbuf->buf[row][col].cell[j] = s[j];
}

void flushFrame(GridCharBuffer* gcbuf) {
    // Compute char_buf byte size and malloc
    int BUFFER_START_OVERHEAD_COST = 9;
    int BUFFER_END_OVERHEAD_COST = 50;
    int ROW_OVERHEAD_COST = 5;
    char BUFFER_START_OVERHEAD_VAL[BUFFER_START_OVERHEAD_COST+1];
    char BUFFER_END_OVERHEAD_VAL[BUFFER_END_OVERHEAD_COST+1];
    char ROW_OVERHEAD_VAL[ROW_OVERHEAD_COST+1];
    strcpy(BUFFER_START_OVERHEAD_VAL, "\x1b[?25l\x1b[H"); // Hide cursor and reset its position
    char tmp_buf[BUFFER_END_OVERHEAD_COST];
    snprintf(tmp_buf, BUFFER_END_OVERHEAD_COST, "\x1b[?25h\x1b[%d;%dH", app_state.cursory + 1, app_state.cursorx + 1); // Show cursor and move it
    strcpy(BUFFER_END_OVERHEAD_VAL, tmp_buf);
    BUFFER_END_OVERHEAD_COST = strlen(BUFFER_END_OVERHEAD_VAL);
    strcpy(ROW_OVERHEAD_VAL, "\x1b[K\r\n"); // Clear line and goto next line
    int char_buf_byte_size = 
        gcbuf->byte_size + 
        ROW_OVERHEAD_COST*gcbuf->rows + 
        BUFFER_START_OVERHEAD_COST +
        BUFFER_END_OVERHEAD_COST;
    char* char_buf = (char*)malloc(sizeof(char)*char_buf_byte_size);

    //char* char_buf = (char*)malloc(sizeof(char)*char_buf_byte_size);

    // Fill char buffer and flush
    int i=0;
    for (int j=0; j<BUFFER_START_OVERHEAD_COST; j++) 
        char_buf[i+j] = BUFFER_START_OVERHEAD_VAL[j];
    i += BUFFER_START_OVERHEAD_COST;

    for (int row=0; row<gcbuf->rows; row++) {
        for (int col=0; col<gcbuf->cols; col++)  {
            for (int j=0; j<gcbuf->buf[row][col].size; j++)
                char_buf[i+j] = gcbuf->buf[row][col].cell[j];
            i += gcbuf->buf[row][col].size;   
            //printf("i=%d | size=%d\r\n", i, gcbuf->buf[row][col].size);           
        }
        for (int j=0; j<ROW_OVERHEAD_COST; j++)
            char_buf[i+j] = ROW_OVERHEAD_VAL[j];
        i += ROW_OVERHEAD_COST;
    }
    // We backtrace the last 2 chars to ommit the last \r\n
    i -= 2;

    for (int j=0; j<BUFFER_END_OVERHEAD_COST; j++) 
        char_buf[i+j] = BUFFER_END_OVERHEAD_VAL[j];
    i += BUFFER_END_OVERHEAD_COST;

    write(STDOUT_FILENO, char_buf, i);
    free(char_buf);
}

void setCursorPos(int row, int col) {
    app_state.cursorx = col;
    app_state.cursory = row;
}

void setCursorPosRelative(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col) {
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, 0, 0);
    setCursorPos(pos_row+offset_row, pos_col+offset_col);
}

void getDrawPosition(int* offset_row, int* offset_col, ScreenPos pos, GridCharBuffer* gcbuf, int width, int height) {
    printf("Woho, getDrawOffset %d\r\n", pos);
    switch (pos) {
    case TOP_LEFT:
        *offset_row = 0;
        *offset_col = 0;
        break;
    case TOP_CENTER:
        *offset_row = 0;
        *offset_col = gcbuf->cols/2 - width/2;
        break;
    case TOP_RIGHT:
        *offset_row = 0;
        *offset_col = gcbuf->cols - width;
        break;
    case CENTER_LEFT:
        *offset_row = gcbuf->rows/2 - height/2;
        *offset_col = 0;
        break;
    case CENTER:
        *offset_row = gcbuf->rows/2 - height/2;
        *offset_col = gcbuf->cols/2 - width/2;
        break;
    case CENTER_RIGHT:
        *offset_row = gcbuf->rows/2 - height/2;
        *offset_col = gcbuf->cols - width;
        break;
    case BOTTOM_LEFT:
        *offset_row = gcbuf->rows - height;
        *offset_col = 0;
        break;
    case BOTTOM_CENTER:
        *offset_row = gcbuf->rows - height;
        *offset_col = gcbuf->cols/2 - width/2;
        break;
    case BOTTOM_RIGHT:
        *offset_row = gcbuf->rows - height;
        *offset_col = gcbuf->cols - width;
        break;

    default:
        *offset_row = 0;
        *offset_col = 0;
        break;
    }
}

// ========= DRAW ========

void drawBox(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos) {
    drawBoxWithOffset(gcbuf, width, height, pos, 0, 0);
}

void drawBoxWithOffset(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos, int offset_row, int offset_col) { 
    // width and height are inset size
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, width+2, height+2);
    pos_row += offset_row;
    pos_col += offset_col;

    for (int row=0; row<height+2; row++) {
        for (int col=0; col<width+2; col++) {
            char box_drawing_char[4];
            if (row==0) {
                if (col==0) strcpy(box_drawing_char, "┌");
                else if (col==width+1) strcpy(box_drawing_char, "┐");
                else strcpy(box_drawing_char, "─");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3);
            }
            else if (row==height+1) {
                if (col==0) strcpy(box_drawing_char, "└");
                else if (col==width+1) strcpy(box_drawing_char, "┘");
                else strcpy(box_drawing_char, "─");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3);
            }
            else if (col==0 || col==width+1) {
                strcpy(box_drawing_char, "│");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3);
            }
        }
    }
}


void drawStrongBox(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos) {
    drawStrongBoxWithOffset(gcbuf, width, height, pos, 0, 0);
}

void drawStrongBoxWithOffset(GridCharBuffer* gcbuf, int width, int height, ScreenPos pos, int offset_row, int offset_col) { 
    // width and height are inset size
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, width+2, height+2);
    pos_row += offset_row;
    pos_col += offset_col;

    for (int row=0; row<height+2; row++) {
        for (int col=0; col<width+2; col++) {
            char box_drawing_char[4];
            if (row==0) {
                if (col==0) strcpy(box_drawing_char, "╔");
                else if (col==width+1) strcpy(box_drawing_char, "╗");
                else strcpy(box_drawing_char, "═");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3);
            }
            else if (row==height+1) {
                if (col==0) strcpy(box_drawing_char, "╚");
                else if (col==width+1) strcpy(box_drawing_char, "╝");
                else strcpy(box_drawing_char, "═");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3);
            }
            else if (col==0 || col==width+1) {
                strcpy(box_drawing_char, "║");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3);
            }
        }
    }
}

// strlen for unicode strings
size_t u_strlen(char *s)
{
    size_t len = 0;
    for (; *s; ++s) if ((*s & 0xC0) != 0x80) ++len;
    return len;
}

// length for one unicode char
size_t u_charlen(char *s)
{
    if (!s) return 0;
    unsigned char c = (unsigned char)*s;

    if (c < 0x80)        // 0xxxxxxx → ASCII (1 octet)
        return 1;
    else if ((c >> 5) == 0x6)  // 110xxxxx → 2 octets
        return 2;
    else if ((c >> 4) == 0xE)  // 1110xxxx → 3 octets
        return 3;
    else if ((c >> 3) == 0x1E) // 11110xxx → 4 octets
        return 4;
    else
        return 1;  // caractère invalide ou continuation → on renvoie 1 par défaut
}

void drawText(GridCharBuffer* gcbuf, char* txt, ScreenPos pos) {
    drawTextWithOffset(gcbuf, txt, pos, 0, 0);
}

void drawTextWithOffset(GridCharBuffer* gcbuf, char* txt, ScreenPos pos, int offset_row, int offset_col) {
    int txt_width = u_strlen(txt);
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, txt_width, 0);
    pos_row += offset_row;
    pos_col += offset_col;
    int i=0;
    int j=0;
    while (txt[j]!='\0') {
        int char_size = u_charlen(&txt[j]);
        putGcbuf(gcbuf, pos_row, pos_col+i, &txt[j], char_size);
        i++;
        j+=char_size;
    }
}

void drawRows(GridCharBuffer* gcbuf) {
    for (int row = 0; row < gcbuf->rows; row++)
        putGcbuf(gcbuf, row, 0, "~", 1);
}

void drawTitle(GridCharBuffer* gcbuf, ScreenPos pos) {
    drawTitleWithOffset(gcbuf, pos, 0, 0);
}
void drawTitleWithOffset(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col) {
    int logo_var = 4;
    switch (logo_var)
    {
    case 1:
        char line1_1[] = "░█▀█░█▀▀░▀█▀░█░█░█▀█░█▀▄░█░█░░░█▀█░█░█░█▀█░█░░░█▀▀";
        char line1_2[] = "░█░█░█▀▀░░█░░█▄█░█░█░█▀▄░█▀▄░░░█▀█░█▄█░█▀█░█░░░█▀▀";
        char line1_3[] = "░▀░▀░▀▀▀░░▀░░▀░▀░▀▀▀░▀░▀░▀░▀░░░▀░▀░▀░▀░▀░▀░▀▀▀░▀▀▀";
        drawTextWithOffset(gcbuf, line1_1, pos, -1+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line1_2, pos, 0+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line1_3, pos, 1+offset_row, 0+offset_col);
        break;
    case 2:
        char line2_1[] = " _______          __                       __        _____                 .__          ";
        char line2_2[] = " \\      \\   _____/  |___  _  _____________|  | __   /  _  \\__  _  _______  |  |   ____   ";
        char line2_3[] = " /   |   \\_/ __ \\   __\\ \\/ \\/ /  _ \\_  __ \\  |/ /  /  /_\\  \\ \\/ \\/ /\\__  \\ |  | _/ __ \\ ";
        char line2_4[] = "/    |    \\  ___/|  |  \\     (  <_> )  | \\/    <  /    |    \\     /  / __ \\|  |_\\  ___/ ";
        char line2_5[] = "\\____|__  /\\___  >__|   \\/\\_/ \\____/|__|  |__|_ \\ \\____|__  /\\/\\_/  (____  /____/\\___  >";
        char line2_6[] = "        \\/     \\/                              \\/         \\/             \\/          \\/ ";
        drawTextWithOffset(gcbuf, line2_1, pos, -2+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line2_2, pos, -1+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line2_3, pos, 0+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line2_4, pos, 1+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line2_5, pos, 2+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line2_6, pos, 3+offset_row, 0+offset_col);
        break;
    case 3:
        char line3_1[] = "███╗   ██╗███████╗████████╗██╗    ██╗ ██████╗ ██████╗ ██╗  ██╗     █████╗ ██╗    ██╗ █████╗ ██╗     ███████╗";
        char line3_2[] = "████╗  ██║██╔════╝╚══██╔══╝██║    ██║██╔═══██╗██╔══██╗██║ ██╔╝    ██╔══██╗██║    ██║██╔══██╗██║     ██╔════╝";
        char line3_3[] = "██╔██╗ ██║█████╗     ██║   ██║ █╗ ██║██║   ██║██████╔╝█████╔╝     ███████║██║ █╗ ██║███████║██║     █████╗  ";
        char line3_4[] = "██║╚██╗██║██╔══╝     ██║   ██║███╗██║██║   ██║██╔══██╗██╔═██╗     ██╔══██║██║███╗██║██╔══██║██║     ██╔══╝  ";
        char line3_5[] = "██║ ╚████║███████╗   ██║   ╚███╔███╔╝╚██████╔╝██║  ██║██║  ██╗    ██║  ██║╚███╔███╔╝██║  ██║███████╗███████╗";
        char line3_6[] = "╚═╝  ╚═══╝╚══════╝   ╚═╝    ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝    ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚══════╝╚══════╝";
        drawTextWithOffset(gcbuf, line3_1, pos, -2+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line3_2, pos, -1+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line3_3, pos, 0+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line3_4, pos, 1+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line3_5, pos, 2+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line3_6, pos, 3+offset_row, 0+offset_col);
        break;
    case 4:
        char line4_1[] = "███╗   ██╗███████╗████████╗██╗    ██╗ ██████╗ ██████╗ ██╗  ██╗";
        char line4_2[] = "████╗  ██║██╔════╝╚══██╔══╝██║    ██║██╔═══██╗██╔══██╗██║ ██╔╝";
        char line4_3[] = "██╔██╗ ██║█████╗     ██║   ██║ █╗ ██║██║   ██║██████╔╝█████╔╝ ";
        char line4_4[] = "██║╚██╗██║██╔══╝     ██║   ██║███╗██║██║   ██║██╔══██╗██╔═██╗ ";
        char line4_5[] = "██║ ╚████║███████╗   ██║   ╚███╔███╔╝╚██████╔╝██║  ██║██║  ██╗";
        char line4_6[] = "╚═╝  ╚═══╝╚══════╝   ╚═╝    ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝";
        char line4_7[] = "            █████╗ ██╗    ██╗ █████╗ ██╗     ███████╗         ";
        char line4_8[] = "           ██╔══██╗██║    ██║██╔══██╗██║     ██╔════╝         ";
        char line4_9[] = "           ███████║██║ █╗ ██║███████║██║     █████╗           ";
        char line4_10[]= "           ██╔══██║██║███╗██║██╔══██║██║     ██╔══╝           ";
        char line4_11[]= "           ██║  ██║╚███╔███╔╝██║  ██║███████╗███████╗         ";
        char line4_12[]= "           ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚══════╝╚══════╝         ";
        char line4_13[]= "     ──────────────────  2   0   0   0  ──────────────────    ";
        drawTextWithOffset(gcbuf, line4_1, pos, -6+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_2, pos, -5+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_3, pos, -4+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_4, pos, -3+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_5, pos, -2+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_6, pos, -1+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_7, pos, 0+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_8, pos, 1+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_9, pos, 2+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_10, pos, 3+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_11, pos, 4+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_12, pos, 5+offset_row, 0+offset_col);
        drawTextWithOffset(gcbuf, line4_13, pos, 6+offset_row, 0+offset_col);
        break;
    
    default:
        drawTextWithOffset(gcbuf, "Network Awalé", pos, offset_row, offset_col);
        break;
    }
}