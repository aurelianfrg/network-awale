#include "tui.h"

// ===================================
// ======== RAW TERMINAL STUFF =======
// ===================================
volatile sig_atomic_t resize_pending = 0;
TerminalConfig term_config;

const TextStyle _NO_STYLE = {0,0,0};
const TextStyle* NO_STYLE = &_NO_STYLE;

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
    // terminalClearScreen();
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
    write(STDOUT_FILENO, "\x1b[?25h\x1b[H\x1b[0m", 13);
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
    int out;
    unsigned char c;
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
                        case '1': return KEY_HOME;
                        case '3': return KEY_DEL;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
            }
        }
        return '\x1b';
    } else {
        if (u_charlen(&c) == 1) {
            out = c;
        }
        else {
            unsigned char uchar[4];
            uchar[0] = c;
            for (int i=1; i<u_charlen(&c); i++) {
                if (read(STDIN_FILENO, uchar+i, 1) != 1) die("Error reading multi-bytes unicode char");
            }
            out = u_bytesToCode(uchar);
        }
        
        return out;
    }
}

void terminalClearScreen() {
    // Clear formatting, clear screen, move cursor to top-left
    write(STDOUT_FILENO, "\x1b[0m\x1b[2J\x1b[H", 11);
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
    app_state.cursor_x = 0;
    app_state.cursor_y = 0;
    app_state.term_config = *initTerminal();
    // Set signal handling
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

void processKeypress(int c) {
    switch (c) {
        case KEY_ARROW_UP:
        case KEY_ARROW_LEFT:
        case KEY_ARROW_DOWN:
        case KEY_ARROW_RIGHT:
        case 'z':
        case 'q':
        case 's':
        case 'd':
            editorMoveCursor(c);
            break;
        case KEY_PAGE_UP:
        case KEY_PAGE_DOWN:
            int times = app_state.term_config.screenrows;
            while (times--)
                editorMoveCursor(c == KEY_PAGE_UP ? KEY_ARROW_UP : KEY_ARROW_DOWN);
            break;
        case KEY_HOME:
            app_state.cursor_x = 0;
            break;
        case KEY_END:
            app_state.cursor_x = app_state.term_config.screencols - 1;
            break;
    }
}

void editorMoveCursor(int key) {
  switch (key) {
    case KEY_ARROW_LEFT:
    case 'q':
        if (app_state.cursor_x != 0) {
            app_state.cursor_x--;
        }
        break;
    case KEY_ARROW_RIGHT:
    case 'd':
        if (app_state.cursor_x != app_state.term_config.screencols - 1) {
            app_state.cursor_x++;
        }
        break;
    case KEY_ARROW_UP:
    case 'z':
        if (app_state.cursor_y != 0) {
            app_state.cursor_y--;
        }
        break;
    case KEY_ARROW_DOWN:
    case 's':
        if (app_state.cursor_y != app_state.term_config.screenrows - 1) {
            app_state.cursor_y++;
        }
        break;
    }
}

// ========= OUTPUT ==========

void drawFrame() {
    app_state.cursor_visibility = 1; // Cursor is visible by default, must be explicitly hidden each frame
    // Resize if needed
    if (resize_pending) {
        resize_pending = 0;
        if (getWindowSize(&app_state.term_config.screenrows, &app_state.term_config.screencols) == -1) die("getWindowSize");
    }
    // Prepare frame
    GridCharBuffer* gcbuf = createGcbuf(app_state.term_config.screenrows, app_state.term_config.screencols);

    // Draw frame
    frameContent(gcbuf);

    // flush frame
    // Block SIGWINCH calls during frame flushing
    sigset_t x,old;
    sigemptyset(&x);
    sigaddset(&x, SIGWINCH);
    int ret = sigprocmask(SIG_BLOCK, &x, &old);
    // exit(1);
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
    for (int row=0; row<rows; row++) {
        buf[row] = (CellChar*)malloc(sizeof(CellChar)*cols);
        for (int col=0; col<cols; col++) {
            putGcbuf(gcbuf, row, col, " ", 1, NO_STYLE);
        }
    }
    return gcbuf;
}

void freeGcbuf(GridCharBuffer* gcbuf) {
    for (int row=0; row<gcbuf->rows; row++) free(gcbuf->buf[row]);
    free(gcbuf->buf);
    free(gcbuf);
}

void putGcbuf(GridCharBuffer* gcbuf, int row, int col, char* s, int bytes, TextStyle* style) {
    // Some error handling
    if (row > gcbuf->rows-1 || col > gcbuf->cols-1) return;
    if (row < 0 || col < 0) return;

    gcbuf->buf[row][col].style.flags = style->flags;
    gcbuf->buf[row][col].style.bg_color_code = style->bg_color_code;
    gcbuf->buf[row][col].style.fg_color_code = style->fg_color_code;
    for (int j=0; j<bytes; j++)
        gcbuf->buf[row][col].uchar[j] = s[j];
}

int getFlagState(char source, char flagPos) {
    int mask = 1<<flagPos;
    return (source & mask) >> flagPos;
}

void setFlagState(char* source, char flagPos, char state) {
    int mask = 1<<flagPos;
    if (state)
        *source = source || mask;
    else
        *source = source && ~mask;
}

char mkStyleFlags(int flag_count,...) {
    char style=0; // tous les flags baissés
    if (flag_count) {
        va_list args; va_start(args, flag_count);
        for (int i = 0; i < flag_count; i++) {
            TextStyleFlags flag = va_arg(args, TextStyleFlags);
            style += 1<<flag;
        }
        va_end(args);
    }
    return style;
}

char addStyleFlag(char style_flags, TextStyleFlags flag) {
    return style_flags | (1<<flag);
}

int getGcbufSize(GridCharBuffer* gcbuf) {
    int size = 0;
    for (int row=0; row<gcbuf->rows; row++) {
        for (int col=0; col<gcbuf->cols; col++) {
            size += getCellCharSize(&(gcbuf->buf[row][col]));
        }
    }
    return size;
}

int getTextStyleSize(TextStyle* text_style) {
    int style_size = 4; // \033...m et éventuel [0
    if (getFlagState(text_style->flags, FG_COLOR)) style_size += 9;  // [38;5;xxx
    if (getFlagState(text_style->flags, BG_COLOR)) style_size += 9;  // [48;5;xxx
    if (getFlagState(text_style->flags, BOLD)) style_size += 2;      // [1
    if (getFlagState(text_style->flags, FAINT)) style_size += 2;     // [2
    if (getFlagState(text_style->flags, ITALIC)) style_size += 2;    // [3
    if (getFlagState(text_style->flags, UNDERLINE)) style_size += 2; // [4
    if (getFlagState(text_style->flags, STRIKETHROUGH)) style_size += 2;     // [5
    if (getFlagState(text_style->flags, INVERSE)) style_size += 2;   // [7
    return style_size;
}

int getCellCharSize(CellChar* cell_char) {
    int size = (int)u_charlen(cell_char->uchar);
    int style_size = getTextStyleSize(&cell_char->style);
    return size + style_size;
    // Plus d'optimisation d'espace est possible en calculant la taille en octets de la valeurs du bg et fg
}

void flushFrame(GridCharBuffer* gcbuf) {
    // Compute char_buf byte size and malloc
    int BUFFER_START_OVERHEAD_COST = 13;
    int BUFFER_END_OVERHEAD_COST = 30;
    int ROW_OVERHEAD_COST = 5;
    char BUFFER_START_OVERHEAD_VAL[BUFFER_START_OVERHEAD_COST+1];
    char BUFFER_END_OVERHEAD_VAL[BUFFER_END_OVERHEAD_COST+1];
    char ROW_OVERHEAD_VAL[ROW_OVERHEAD_COST+1];
    strcpy(BUFFER_START_OVERHEAD_VAL, "\x1b[?25l\x1b[H\x1b[0m"); // Hide cursor and reset its position
    snprintf(BUFFER_END_OVERHEAD_VAL, BUFFER_END_OVERHEAD_COST, "\x1b[?25%c\x1b[%d;%dH", (app_state.cursor_visibility)?'h':'l', app_state.cursor_y + 1, app_state.cursor_x + 1); // Show cursor and move it
    BUFFER_END_OVERHEAD_COST = strlen(BUFFER_END_OVERHEAD_VAL);
    strcpy(ROW_OVERHEAD_VAL, "\x1b[K\r\n"); // Clear end of line and goto next line
    int gcbuf_size = 0;
    int char_buf_byte_size = 
        getGcbufSize(gcbuf) + 
        ROW_OVERHEAD_COST*gcbuf->rows + 
        BUFFER_START_OVERHEAD_COST +
        BUFFER_END_OVERHEAD_COST;
    char* char_buf = (char*)malloc(sizeof(char)*char_buf_byte_size);

    // Fill char buffer and flush
    int i=0;
    for (int j=0; j<BUFFER_START_OVERHEAD_COST; j++) 
        char_buf[i+j] = BUFFER_START_OVERHEAD_VAL[j];
    i += BUFFER_START_OVERHEAD_COST;
    char prev_style_flags = 0;
    char prev_fg_color = 0;
    char prev_bg_color = 0;

    for (int row=0; row<gcbuf->rows; row++) {
        for (int col=0; col<gcbuf->cols; col++)  {
            if (prev_style_flags != gcbuf->buf[row][col].style.flags || 
                getFlagState(gcbuf->buf[row][col].style.flags, FG_COLOR) && prev_fg_color != gcbuf->buf[row][col].style.fg_color_code ||
                getFlagState(gcbuf->buf[row][col].style.flags, BG_COLOR) && prev_bg_color != gcbuf->buf[row][col].style.bg_color_code) 
            {
                char style_buf[34]; // Taille maximale du buffer si tous les styles sont mit + reset sequence
                int offset = sprintf(style_buf, "\x1b[0");
                if (getFlagState(gcbuf->buf[row][col].style.flags, FG_COLOR)) offset += sprintf(style_buf+offset, ";38;5;%d", gcbuf->buf[row][col].style.fg_color_code);
                if (getFlagState(gcbuf->buf[row][col].style.flags, BG_COLOR)) offset += sprintf(style_buf+offset, ";48;5;%d", gcbuf->buf[row][col].style.bg_color_code);
                if (getFlagState(gcbuf->buf[row][col].style.flags, BOLD)) offset += sprintf(style_buf+offset, ";1");
                if (getFlagState(gcbuf->buf[row][col].style.flags, FAINT)) offset += sprintf(style_buf+offset, ";2");
                if (getFlagState(gcbuf->buf[row][col].style.flags, ITALIC)) offset += sprintf(style_buf+offset, ";3");
                if (getFlagState(gcbuf->buf[row][col].style.flags, UNDERLINE)) offset += sprintf(style_buf+offset, ";4");
                if (getFlagState(gcbuf->buf[row][col].style.flags, STRIKETHROUGH)) offset += sprintf(style_buf+offset, ";9");
                if (getFlagState(gcbuf->buf[row][col].style.flags, INVERSE)) offset += sprintf(style_buf+offset, ";7");
                offset += sprintf(style_buf+offset, "m");
                for (int j=0; j<offset; j++)
                    char_buf[i+j] = style_buf[j];
                i += offset;
            }
            prev_style_flags = gcbuf->buf[row][col].style.flags;
            prev_fg_color = gcbuf->buf[row][col].style.fg_color_code;
            prev_bg_color = gcbuf->buf[row][col].style.bg_color_code;

            int size = u_charlen(gcbuf->buf[row][col].uchar);
            for (int j=0; j<size; j++)
                char_buf[i+j] = gcbuf->buf[row][col].uchar[j];
            i += size;   
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

    if (i>char_buf_byte_size) {
        printf("i=%d et char_buf_byte_size=%d\r\n", i, char_buf_byte_size);
        die("\r\ni est plus grand que char_buf_byte_size");
    }

    write(STDOUT_FILENO, char_buf, i);
    free(char_buf);
}

void setCursorPos(int row, int col) {
    app_state.cursor_x = col;
    app_state.cursor_y = row;
}

void hideCursor() {
    app_state.cursor_visibility = 0;
}

void showCursor() {
    app_state.cursor_visibility = 1;
}

void setCursorPosRelative(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col) {
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, 0, 0);
    setCursorPos(pos_row+offset_row, pos_col+offset_col);
}

void getDrawPosition(int* offset_row, int* offset_col, ScreenPos pos, GridCharBuffer* gcbuf, int width, int height) {
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

void drawBox(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int width, int height) { 
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
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3, style);
            }
            else if (row==height+1) {
                if (col==0) strcpy(box_drawing_char, "└");
                else if (col==width+1) strcpy(box_drawing_char, "┘");
                else strcpy(box_drawing_char, "─");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3, style);
            }
            else if (col==0 || col==width+1) {
                strcpy(box_drawing_char, "│");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3, style);
            }
        }
    }
}

void drawStrongBox(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int width, int height) { 
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
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3, style);
            }
            else if (row==height+1) {
                if (col==0) strcpy(box_drawing_char, "╚");
                else if (col==width+1) strcpy(box_drawing_char, "╝");
                else strcpy(box_drawing_char, "═");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3, style);
            }
            else if (col==0 || col==width+1) {
                strcpy(box_drawing_char, "║");
                putGcbuf(gcbuf, pos_row+row, pos_col+col, box_drawing_char, 3, style);
            }
        }
    }
}

size_t u_strlen(char *s)
{
    size_t len = 0;
    for (; *s; ++s) if ((*s & 0xC0) != 0x80) ++len;
    return len;
}

size_t u_charlen(char *s)
{
    if (!s) return 0;
    unsigned char c = (unsigned char)*s;

    if (c < 0x80)        // 0xxxxxxx → ASCII (1 octet)
        return (size_t)1;
    else if ((c >> 5) == 0x6)  // 110xxxxx → 2 octets
        return (size_t)2;
    else if ((c >> 4) == 0xE)  // 1110xxxx → 3 octets
        return (size_t)3;
    else if ((c >> 3) == 0x1E) // 11110xxx → 4 octets
        return (size_t)4;
    else
        return (size_t)0;  // caractère invalide ou continuation → on renvoie 0 par défaut
}

int isAlpha(int c) {
    return (c>=97 && c<=122) || (c>=65 && c<=90);
}

int isNumeric(int c) {
    return c>=48 && c<=57;
}

int isAlphaNumeric(int c) {
    return isAlpha(c) || isNumeric(c);
}

int isAnyValidChar(int c) {
    // A subset of accepted unicode values for string (prevents to print unprintables chars)
    return (c>=32 && c<=126) || (c>=128 && c<=255 && c!=129 && c!=141 && c!=143 && c!=144 && c!=157);; 
}

int isAnyAsciiChar(int c) {
    return (c>=32 && c<=126);
}

int u_strAppend(u_string* s, int codepoint) {
    char uchar[4];
    s->char_len++;
    int uchar_len = u_codeToBytes(codepoint, uchar);
    for (int i=0; i<uchar_len; i++) {
        s->buf[s->byte_len] = uchar[i];
        s->byte_len++;
    }
    s->buf[s->byte_len] = '\0';
    return uchar_len;
}

int u_strPop(u_string* s) {
    if (s->byte_len > 0) {
        do {
            s->byte_len--;
        } while (s->byte_len>0 && !u_charlen(&s->buf[s->byte_len]));
        s->buf[s->byte_len] = '\0';
        s->char_len--;
    }
}

int u_codeToBytes(unsigned int c, unsigned char* out) {
    // Returns the size of the char out
    if (c < 128) { // 2^8
        out[0] = c;
        return 1;
    }
    else if (c < 2048) {
        out[0] = (((c&0xFFFC0)>>6)+0xC0); // Mask the 6 first bits, should get 5 bits
        out[1] = ((c&0x3F)+0x80); // Get the last 6 bits
        return 2;
    } else if (c < 65536) {
        out[0] = (((c&0xFFE000)>>12)+0xE0); // Mask the 12 first bits, should get 4 bits
        out[1] = (((c&0x0FC0)>>6)+0x80); // Mask the 6 first bits, should get the 6 middle bits
        out[2] = ((c&0x3F)+0x80); // Last 6 bits
        return 3;
    } else if (c < 2097152) {
        out[0] = (((c&0xFC0000)>>18)+0xF0); // Mask the 18 first bits, should get 3 bits
        out[1] = (((c&0x3F000)>>12)+0x80); // Mask the 12 first bits, should get the 6 middle bits
        out[2] = (((c&0x0FC0)>>6)+0x80); // Mask the 6 first bits, should get the 6 middle bits
        out[3] = ((c&0x3F)+0x80); // Last 6 bits
        return 4;
    }
}

int u_bytesToCode(unsigned char* char_in) {
    // Returns the unicode codepoint
    int u_len = (int)u_charlen(char_in);
    char buf[300];
    if (u_len == 1) return *char_in;

    int first_byte_prefix;
    int continuation_byte_prefix = 0x80;
    int codepoint = 0;
    if (u_len == 2) first_byte_prefix = 0xC0;
    if (u_len == 3) first_byte_prefix = 0xE0;
    if (u_len == 4) first_byte_prefix = 0xF0;

    codepoint += (char_in[0]-first_byte_prefix) * (1 << (6*(u_len-1)));
    sprintf(buf, "codepoint: %d, 1 << 6*(u_len-1): %d, char_in[0]: %d\r\n", codepoint, 1 << (6*(u_len-1)), char_in[0]);
    for (int i=1; i<u_len; i++)
        codepoint += (char_in[i]-continuation_byte_prefix) * (1 << (6*((u_len-i)-1)));
    
    return codepoint;
}

void drawText(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, const char* text) {
    // Iterate the string to get its size and pos
    int txt_width = u_strlen(text);
    for (int i=0; text[i]!='\0'; i++) {
        if (text[i]== '!' && text[i+1]=='{') {
            i+=2;
            txt_width -= 3;
            while (text[i]!='}') {
                switch (text[i]) {
                case 'F': // Set fg, checks next 3 vals for color
                case 'B': // Set bg, checks next 3 vals for color
                    for (int j=1; j<4; j++)
                        if (text[i+j]-'0'<0 || text[i+j]-'0'>9) die("F or B flag should be folled by 3 digits (drawText)");
                    txt_width -= 4;
                    i+=3;
                    break;
                case 'b': // Set to bold
                case 'f': // Set to faint
                case 'i': // Set to italic
                case 'u': // Set to underline
                case 'v': // Set to inverse
                case 's': // Set to strikethrough
                case 'r': // Clear all style
                    txt_width -= 1;
                    break;
                default: // any other char, should not happen
                    char buf[34];
                    sprintf(buf, "Invalid text format: %c (drawText)", text[i]);
                    dieNoError(buf);
                    break;
                }
                i++;
            }
            i++;
        }
    }
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, txt_width, 0);
    pos_row += offset_row;
    pos_col += offset_col;
    int row = 0;
    // Iterate again to build the result
    TextStyle style = { 0, 0, 0 };
    int i=0;
    while(text[i]!='\0') {
        switch (text[i]){
        case '!':
            if (text[i+1]=='{') {
                i+=2; // Skips "!{"
                while (text[i]!='}') {
                    switch (text[i]) {
                    case 'F': // Set fg, checks next 3 vals for color
                        style.fg_color_code = (text[i+1]-'0')*100 + (text[i+2]-'0')*10 + (text[i+3]-'0');
                        style.flags = addStyleFlag(style.flags, FG_COLOR);
                        i+=3;
                        break;
                    case 'B': // Set bg, checks next 3 vals for color
                        style.bg_color_code = (text[i+1]-'0')*100 + (text[i+2]-'0')*10 + (text[i+3]-'0');
                        style.flags = addStyleFlag(style.flags, BG_COLOR);
                        i+=3;
                        break;
                    case 'b': // Set to bold
                        style.flags = addStyleFlag(style.flags, BOLD);
                        break;
                    case 'f': // Set to faint
                        style.flags = addStyleFlag(style.flags, FAINT);
                        break;
                    case 'i': // Set to italic
                        style.flags = addStyleFlag(style.flags, ITALIC);
                        break;
                    case 'u': // Set to underline
                        style.flags = addStyleFlag(style.flags, UNDERLINE);
                        break;
                    case 'v': // Set to inverse
                        style.flags = addStyleFlag(style.flags, INVERSE);
                        break;
                    case 's': // Set to strikethrough
                        style.flags = addStyleFlag(style.flags, STRIKETHROUGH);
                        break;
                    case 'r': // Clear all style
                        style.flags = 0;
                        break;
                    }
                    i++; // next symbol
                }
                i++; // Skips "}" 
            }
            else {
                int char_size = u_charlen(&text[i]);
                putGcbuf(gcbuf, pos_row, pos_col+row, &text[i], char_size, &style);
                i+=char_size;
                row++;
            }
            break;
        default:
            int char_size = u_charlen(&text[i]);
            putGcbuf(gcbuf, pos_row, pos_col+row, &text[i], char_size, &style);
            i+=char_size;
            row++;
            break;
        }
    }
}

void drawTextWithRawStyle(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, const char* text, TextStyle* style) {
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, u_strlen(text), 0);
    pos_row += offset_row;
    pos_col += offset_col;
    int row = 0;
    int i=0;
    while (text[i]!='\0') {
        int char_size = u_charlen(&text[i]);
        putGcbuf(gcbuf, pos_row, pos_col+row, &text[i], char_size, style);
        i+=char_size;
        row++;
    }
}

void drawButton(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, const char* text, unsigned char color_code, int selected) {
    size_t text_length = u_strlen(text);
    char buf[text_length+13];
    if (selected) sprintf(buf, "!{iB%03d}[<%s>]", color_code, text);
    else sprintf(buf, "!{iF%03d}[<%s>]", color_code, text);
    
    drawText(gcbuf, pos, offset_row, offset_col, buf);
}

void drawDebugColors(GridCharBuffer* gcbuf) {
    int COLOR_MAX = 256;
    int COL_WIDTH = 15;
    int row=1;
    int col=0;
    int color = 0;
    drawText(gcbuf, TOP_LEFT, 0, 0, "Color codes:");
    while (color < COLOR_MAX) {
        TextStyle style = { 0, color, color };
        char buf[4];
        sprintf(buf, "%d", color);
        drawText(gcbuf, TOP_LEFT, row, col, buf);
        style.flags = mkStyleFlags(1, FG_COLOR);                putGcbuf(gcbuf, row, col+3, "a", 1, &style);
        style.flags = mkStyleFlags(2, FG_COLOR, BOLD);          putGcbuf(gcbuf, row, col+4, "a", 1, &style);
        style.flags = mkStyleFlags(2, FG_COLOR, FAINT);         putGcbuf(gcbuf, row, col+5, "a", 1, &style);
        style.flags = mkStyleFlags(1, FAINT);                   putGcbuf(gcbuf, row, col+6, "a", 1, &style);
        style.flags = mkStyleFlags(1, BOLD);                    putGcbuf(gcbuf, row, col+7, "a", 1, &style);
        style.flags = mkStyleFlags(0);                          putGcbuf(gcbuf, row, col+8, "a", 1, &style);
        style.flags = mkStyleFlags(1, BG_COLOR);                putGcbuf(gcbuf, row, col+9, "a", 1, &style);
        style.flags = mkStyleFlags(2, BG_COLOR, FG_COLOR);      putGcbuf(gcbuf, row, col+10, "a", 1, &style);
        style.flags = mkStyleFlags(2, ITALIC, UNDERLINE);       putGcbuf(gcbuf, row, col+11, "a", 1, &style);
        style.flags = mkStyleFlags(2, STRIKETHROUGH, INVERSE);  putGcbuf(gcbuf, row, col+12, "a", 1, &style);
        color++;
        row++;

        if (row >= gcbuf->rows) {
            row=0;
            col+=COL_WIDTH;
        }
    }
}

void drawSolidRect(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, int width, int height, TextStyle* style) {
    int pos_row, pos_col;
    getDrawPosition(&pos_row, &pos_col, pos, gcbuf, width+2, height+2);
    pos_row += offset_row;
    pos_col += offset_col;

    for (int row=0; row<height; row++) {
        for (int col=0; col<width; col++) {
            putGcbuf(gcbuf, pos_row+row, pos_col+col, " ", 1, style);
        }
    }
}

void drawTitle(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col) {
    int logo_var = 4;
    switch (logo_var)
    {
    case 1:
        char line1_1[] = "░█▀█░█▀▀░▀█▀░█░█░█▀█░█▀▄░█░█░░░█▀█░█░█░█▀█░█░░░█▀▀";
        char line1_2[] = "░█░█░█▀▀░░█░░█▄█░█░█░█▀▄░█▀▄░░░█▀█░█▄█░█▀█░█░░░█▀▀";
        char line1_3[] = "░▀░▀░▀▀▀░░▀░░▀░▀░▀▀▀░▀░▀░▀░▀░░░▀░▀░▀░▀░▀░▀░▀▀▀░▀▀▀";
        drawText(gcbuf, pos, -1+offset_row, 0+offset_col, line1_1);
        drawText(gcbuf, pos, 0+offset_row, 0+offset_col, line1_2);
        drawText(gcbuf, pos, 1+offset_row, 0+offset_col, line1_3);
        break;
    case 2:
        char line2_1[] = " _______          __                       __        _____                 .__          ";
        char line2_2[] = " \\      \\   _____/  |___  _  _____________|  | __   /  _  \\__  _  _______  |  |   ____   ";
        char line2_3[] = " /   |   \\_/ __ \\   __\\ \\/ \\/ /  _ \\_  __ \\  |/ /  /  /_\\  \\ \\/ \\/ /\\__  \\ |  | _/ __ \\ ";
        char line2_4[] = "/    |    \\  ___/|  |  \\     (  <_> )  | \\/    <  /    |    \\     /  / __ \\|  |_\\  ___/ ";
        char line2_5[] = "\\____|__  /\\___  >__|   \\/\\_/ \\____/|__|  |__|_ \\ \\____|__  /\\/\\_/  (____  /____/\\___  >";
        char line2_6[] = "        \\/     \\/                              \\/         \\/             \\/          \\/ ";
        drawText(gcbuf, pos, -2+offset_row, 0+offset_col, line2_1);
        drawText(gcbuf, pos, -1+offset_row, 0+offset_col, line2_2);
        drawText(gcbuf, pos, 0+offset_row, 0+offset_col, line2_3);
        drawText(gcbuf, pos, 1+offset_row, 0+offset_col, line2_4);
        drawText(gcbuf, pos, 2+offset_row, 0+offset_col, line2_5);
        drawText(gcbuf, pos, 3+offset_row, 0+offset_col, line2_6);
        break;
    case 3:
        char line3_1[] = "███╗   ██╗███████╗████████╗██╗    ██╗ ██████╗ ██████╗ ██╗  ██╗     █████╗ ██╗    ██╗ █████╗ ██╗     ███████╗";
        char line3_2[] = "████╗  ██║██╔════╝╚══██╔══╝██║    ██║██╔═══██╗██╔══██╗██║ ██╔╝    ██╔══██╗██║    ██║██╔══██╗██║     ██╔════╝";
        char line3_3[] = "██╔██╗ ██║█████╗     ██║   ██║ █╗ ██║██║   ██║██████╔╝█████╔╝     ███████║██║ █╗ ██║███████║██║     █████╗  ";
        char line3_4[] = "██║╚██╗██║██╔══╝     ██║   ██║███╗██║██║   ██║██╔══██╗██╔═██╗     ██╔══██║██║███╗██║██╔══██║██║     ██╔══╝  ";
        char line3_5[] = "██║ ╚████║███████╗   ██║   ╚███╔███╔╝╚██████╔╝██║  ██║██║  ██╗    ██║  ██║╚███╔███╔╝██║  ██║███████╗███████╗";
        char line3_6[] = "╚═╝  ╚═══╝╚══════╝   ╚═╝    ╚══╝╚══╝  ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝    ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚══════╝╚══════╝";
        drawText(gcbuf, pos, -2+offset_row, 0+offset_col, line3_1);
        drawText(gcbuf, pos, -1+offset_row, 0+offset_col, line3_2);
        drawText(gcbuf, pos, 0+offset_row, 0+offset_col, line3_3);
        drawText(gcbuf, pos, 1+offset_row, 0+offset_col, line3_4);
        drawText(gcbuf, pos, 2+offset_row, 0+offset_col, line3_5);
        drawText(gcbuf, pos, 3+offset_row, 0+offset_col, line3_6);
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
        char line4_13[]= "!{F005}     ──────────────────  2   0   0   0  ──────────────────    ";
        drawText(gcbuf, pos, -6+offset_row, 0+offset_col, line4_1);
        drawText(gcbuf, pos, -5+offset_row, 0+offset_col, line4_2);
        drawText(gcbuf, pos, -4+offset_row, 0+offset_col, line4_3);
        drawText(gcbuf, pos, -3+offset_row, 0+offset_col, line4_4);
        drawText(gcbuf, pos, -2+offset_row, 0+offset_col, line4_5);
        drawText(gcbuf, pos, -1+offset_row, 0+offset_col, line4_6);
        drawText(gcbuf, pos, 0+offset_row, 0+offset_col, line4_7);
        drawText(gcbuf, pos, 1+offset_row, 0+offset_col, line4_8);
        drawText(gcbuf, pos, 2+offset_row, 0+offset_col, line4_9);
        drawText(gcbuf, pos, 3+offset_row, 0+offset_col, line4_10);
        drawText(gcbuf, pos, 4+offset_row, 0+offset_col, line4_11);
        drawText(gcbuf, pos, 5+offset_row, 0+offset_col, line4_12);
        drawText(gcbuf, pos, 6+offset_row, 0+offset_col, line4_13);
        break;
    
    default:
        drawText(gcbuf, pos, offset_row, offset_col, "Network Awalé");
        break;
    }
}

void drawPopup(GridCharBuffer* gcbuf, ScreenPos pos, int offset_row, int offset_col, TextStyle* style, int width, int height, const char* text) {
    drawSolidRect(gcbuf, pos, offset_row+1, offset_col+1, width, height, style);
    drawStrongBox(gcbuf, pos, offset_row, offset_col, style, width, height);
    drawText(gcbuf, pos, offset_row, offset_col, text);
}