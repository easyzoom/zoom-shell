/**
 * @file zoom_shell_2048.c
 * @brief Zoom Shell 2048 游戏
 *
 * 适配自 mevdschee/2048.c (MIT License)
 * 命令: 2048 [blackwhite|bluered]
 * 控制: wasd / 方向键, q=退出, r=重开
 */

#include "zoom_shell.h"

#if ZOOM_USING_GAME

#define SIZE 4

static uint8_t  g_board[SIZE][SIZE];
static uint32_t g_score;
static uint8_t  g_scheme;

static char game_getchar(zoom_shell_t *shell)
{
    char c;
    if (shell->read && shell->read(&c, 1) == 1) return c;
    return (char)-1;
}

static uint32_t simple_rand_state = 12345;
static uint32_t simple_rand(void)
{
    simple_rand_state ^= simple_rand_state << 13;
    simple_rand_state ^= simple_rand_state >> 17;
    simple_rand_state ^= simple_rand_state << 5;
    return simple_rand_state;
}

static void draw_board(zoom_shell_t *shell)
{
    zoom_printf(shell, "\033[H");
    zoom_printf(shell, "  2048.c %17lu pts\r\n\r\n", (unsigned long)g_score);

    for (uint8_t y = 0; y < SIZE; y++) {
        zoom_printf(shell, "  ");
        for (uint8_t x = 0; x < SIZE; x++) {
            if (g_board[x][y] != 0) {
                uint32_t val = (uint32_t)1 << g_board[x][y];
                zoom_printf(shell, " %5lu ", (unsigned long)val);
            } else {
                zoom_printf(shell, "   .   ");
            }
        }
        zoom_printf(shell, "\r\n");
    }
    zoom_printf(shell, "\r\n    wasd or q\r\n");
}

static uint8_t find_target(uint8_t array[SIZE], uint8_t x, uint8_t stop)
{
    if (x == 0) return x;
    for (uint8_t t = x - 1; ; t--) {
        if (array[t] != 0) {
            return (array[t] != array[x]) ? t + 1 : t;
        } else if (t == stop) {
            return t;
        }
    }
    return x;
}

static uint8_t slide_array(uint8_t array[SIZE])
{
    uint8_t success = 0, stop = 0;
    for (uint8_t x = 0; x < SIZE; x++) {
        if (array[x] != 0) {
            uint8_t t = find_target(array, x, stop);
            if (t != x) {
                if (array[t] == 0) {
                    array[t] = array[x];
                } else if (array[t] == array[x]) {
                    array[t]++;
                    g_score += (uint32_t)1 << array[t];
                    stop = t + 1;
                }
                array[x] = 0;
                success = 1;
            }
        }
    }
    return success;
}

static void rotate_board(void)
{
    for (uint8_t i = 0; i < SIZE / 2; i++) {
        for (uint8_t j = i; j < SIZE - i - 1; j++) {
            uint8_t tmp = g_board[i][j];
            g_board[i][j] = g_board[j][SIZE-i-1];
            g_board[j][SIZE-i-1] = g_board[SIZE-i-1][SIZE-j-1];
            g_board[SIZE-i-1][SIZE-j-1] = g_board[SIZE-j-1][i];
            g_board[SIZE-j-1][i] = tmp;
        }
    }
}

static uint8_t move_up(void)
{
    uint8_t s = 0;
    for (uint8_t x = 0; x < SIZE; x++) s |= slide_array(g_board[x]);
    return s;
}

static uint8_t move_left(void) { rotate_board(); uint8_t s = move_up(); rotate_board(); rotate_board(); rotate_board(); return s; }
static uint8_t move_down(void) { rotate_board(); rotate_board(); uint8_t s = move_up(); rotate_board(); rotate_board(); return s; }
static uint8_t move_right(void) { rotate_board(); rotate_board(); rotate_board(); uint8_t s = move_up(); rotate_board(); return s; }

static uint8_t count_empty(void)
{
    uint8_t c = 0;
    for (uint8_t x = 0; x < SIZE; x++)
        for (uint8_t y = 0; y < SIZE; y++)
            if (g_board[x][y] == 0) c++;
    return c;
}

static uint8_t find_pair_down(void)
{
    for (uint8_t x = 0; x < SIZE; x++)
        for (uint8_t y = 0; y < SIZE - 1; y++)
            if (g_board[x][y] == g_board[x][y+1]) return 1;
    return 0;
}

static uint8_t game_ended(void)
{
    if (count_empty() > 0) return 0;
    if (find_pair_down()) return 0;
    rotate_board();
    uint8_t r = find_pair_down();
    rotate_board(); rotate_board(); rotate_board();
    return !r;
}

static void add_random(void)
{
    uint8_t list[SIZE*SIZE][2];
    uint8_t len = 0;
    for (uint8_t x = 0; x < SIZE; x++)
        for (uint8_t y = 0; y < SIZE; y++)
            if (g_board[x][y] == 0) { list[len][0] = x; list[len][1] = y; len++; }
    if (len > 0) {
        uint8_t r = simple_rand() % len;
        g_board[list[r][0]][list[r][1]] = (simple_rand() % 10) < 9 ? 1 : 2;
    }
}

static void init_board(zoom_shell_t *shell)
{
    for (uint8_t x = 0; x < SIZE; x++)
        for (uint8_t y = 0; y < SIZE; y++)
            g_board[x][y] = 0;
    g_score = 0;
    add_random();
    add_random();
    draw_board(shell);
}

static int cmd_2048(zoom_shell_t *shell, int argc, char *argv[])
{
    (void)argc; (void)argv;
    g_scheme = 0;
    simple_rand_state = ZOOM_GET_TICK() ^ 0xDEAD;
    if (simple_rand_state == 0) simple_rand_state = 42;

    zoom_printf(shell, "\033[?25l\033[2J");
    init_board(shell);

    while (1) {
        char c = game_getchar(shell);
        if (c == (char)-1) break;

        uint8_t success = 0;
        switch (c) {
        case 'w': case 'A': success = move_up(); break;
        case 's': case 'B': success = move_down(); break;
        case 'a': case 'D': success = move_left(); break;
        case 'd': case 'C': success = move_right(); break;
        case 'q':
            zoom_printf(shell, "\033[?25h\033[m");
            return 0;
        case 'r':
            init_board(shell);
            continue;
        default: continue;
        }

        if (success) {
            draw_board(shell);
            add_random();
            draw_board(shell);
            if (game_ended()) {
                zoom_printf(shell, "         GAME OVER          \r\n");
                game_getchar(shell);
                break;
            }
        }
    }

    zoom_printf(shell, "\033[?25h\033[m");
    return 0;
}

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD(game_2048, cmd_2048, "Play 2048 game",
                ZOOM_ATTR_DEFAULT, ZOOM_USER_GUEST);
#endif

#endif /* ZOOM_USING_GAME */
