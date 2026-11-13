#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define W 30
#define H 15

volatile sig_atomic_t running = 1;
void stop(int sig) { (void)sig; running = 0; }

void food(int *fx, int *fy, int x[], int y[], int n) {
    int i;
    do {
        *fx = rand() % W;
        *fy = rand() % H;
        for (i = 0; i < n && (x[i] != *fx || y[i] != *fy); i++);
    } while (i < n);
}

int main(void) {
    struct termios old, raw;
    if (!isatty(0) || tcgetattr(0, &old) == -1) {
        fprintf(stderr, "Run this in a terminal.\n");
        return 1;
    }
    raw = old;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &raw) == -1) return 1;
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    srand((unsigned)time(NULL));

    int x[W * H], y[W * H], n = 3, fx, fy;
    int dx = 1, dy = 0, nx = 1, ny = 0, esc = 0, win = 0;
    for (int i = 0; i < n; i++) { x[i] = W / 2 - i; y[i] = H / 2; }
    food(&fx, &fy, x, y, n);
    printf("\033[2J\033[?25l");

    while (running) {
        char board[H][W];
        memset(board, ' ', sizeof board);
        board[fy][fx] = '*';
        for (int i = n - 1; i >= 0; i--) board[y[i]][x[i]] = i ? 'o' : '@';
        printf("\033[HScore: %d   Move: arrows / WASD   Quit: q\n+", n - 3);
        for (int i = 0; i < W; i++) putchar('-');
        puts("+");
        for (int j = 0; j < H; j++) {
            putchar('|');
            for (int i = 0; i < W; i++) putchar(board[j][i]);
            puts("|");
        }
        putchar('+');
        for (int i = 0; i < W; i++) putchar('-');
        puts("+");
        fflush(stdout);

        struct timespec delay = {0, 150000000};
        nanosleep(&delay, NULL);
        unsigned char c;
        while (read(0, &c, 1) == 1) {
            if (c == 27) { esc = 1; continue; }
            if (esc == 1 && c == '[') { esc = 2; continue; }
            if (esc == 2) {
                c = c == 'A' ? 'w' : c == 'B' ? 's' : c == 'C' ? 'd' : c == 'D' ? 'a' : 0;
            }
            esc = 0;
            if (c == 'q') { running = 0; break; }
            int ax = nx, ay = ny;
            if (c == 'w') { ax = 0; ay = -1; }
            if (c == 's') { ax = 0; ay =  1; }
            if (c == 'a') { ax = -1; ay = 0; }
            if (c == 'd') { ax =  1; ay = 0; }
            if (ax != -dx || ay != -dy) { nx = ax; ny = ay; }
        }
        if (!running) break;
        dx = nx; dy = ny;
        int hx = x[0] + dx, hy = y[0] + dy;
        int grow = hx == fx && hy == fy;
        if (hx < 0 || hx >= W || hy < 0 || hy >= H) break;
        int i;
        for (i = 0; i < n - 1 + grow; i++)
            if (x[i] == hx && y[i] == hy) break;
        if (i < n - 1 + grow) break;
        for (i = n - 1 + grow; i > 0; i--) { x[i] = x[i - 1]; y[i] = y[i - 1]; }
        x[0] = hx; y[0] = hy;
        n += grow;
        if (grow) {
            if (n == W * H) { win = 1; break; }
            food(&fx, &fy, x, y, n);
        }
    }
    tcsetattr(0, TCSANOW, &old);
    printf("\033[?25h\n%s  Score: %d\n", win ? "You win!" : running ? "Game over!" : "Quit.", n - 3);
    return 0;
}
